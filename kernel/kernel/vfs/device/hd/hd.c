#include "kernel/debug.h"
#include "kernel/drivers/ata.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/task/wait.h"
#include "kernel/vfs/vfs.h"
#include "stdlib.h"
#include <stddef.h>
#include <string.h>
#include "../../rw_queue/rw_queue.h"
#include "../../device.h"
#include "./hd.h"

#define _debug_puts(...) debug_puts_c("VFS/dev/hd", __VA_ARGS__)
#define _debug_printf(...) debug_printf_c("VFS/dev/hd", __VA_ARGS__)

static rw_queue_t rw_queue = RW_QUEUE_INIT;

static void rw_ready(void* ptr)
{
    hd_wakeup_data_t* data = ptr;
    size_t id = data->wait_idx;
    if (!rwq_set_ready(&rw_queue, id)) {
        abort();
    }
    wait_wake_up(data->hd_metadata->wait_obj);
}

static bool rw_wait_for_ready(void* ptr)
{
    hd_wakeup_data_t* data = ptr;
    size_t id = data->wait_idx;
    return rwq_is_ready(&rw_queue, id);
}

static size_t read(
    vfs_file_t* file,
    size_t offset,
    size_t size,
    void* ptr
) {
    if (!file->readable) {
        return 0;
    }

    hd_metadata_t* meta = file->dentry->inode->metadata;

    size_t wait_idx = rwq_allocate_pos(&rw_queue);
    hd_wakeup_data_t wakeup_data;
    wakeup_data.wait_idx = wait_idx;
    wakeup_data.hd_metadata = meta;

    _debug_printf(
        "Received READ call for offset=%u size=%u, wait_idx=%u\n",
        offset,
        size,
        wait_idx
    );

    hd_sector_location_t loc;
    hd_calc_sector_loc(&loc, offset, size, meta);
    if (loc.size == 0) {
        rwq_deallocate_pos(&rw_queue, wait_idx);
        return 0;
    }

    _debug_printf(
        "[wait_idx=%u] lba=%u, sector count=%u\n",
        wait_idx,
        loc.low_sector_lba,
        loc.sector_count
    );

    uint16_t* buffer = kmalloc_flags(
        loc.sector_size * loc.sector_count,
        KMALLOC_ZERO
    );

    ata_read(
        meta->disk_id,
        loc.low_sector_lba,
        loc.sector_count,
        buffer,
        rw_ready,
        &wakeup_data
    );

    _debug_printf("[wait_idx=%u] start waiting\n", wait_idx);

    wait_on_condition(meta->wait_obj, rw_wait_for_ready, &wakeup_data);

    _debug_printf("[wait_idx=%u] waiting completed\n", wait_idx);

    size_t offset_in_buffer = offset - loc.lowest_sector_byte;
    memcpy(ptr, ((uint8_t*)buffer) + offset_in_buffer, loc.size);

    rwq_deallocate_pos(&rw_queue, wait_idx);

    kfree(buffer);
    return loc.size;
}

static size_t write(
    vfs_file_t* file,
    size_t offset,
    size_t size,
    const void* ptr
) {
    if (!file->writeable) {
        return 0;
    }

    hd_metadata_t* meta = file->dentry->inode->metadata;

    size_t wait_idx = rwq_allocate_pos(&rw_queue);
    hd_wakeup_data_t wakeup_data;
    wakeup_data.wait_idx = wait_idx;
    wakeup_data.hd_metadata = meta;

    _debug_printf(
        "Received WRITE call for offset=%u size=%u, wait_idx=%u\n",
        offset,
        size,
        wait_idx
    );

    hd_sector_location_t loc;
    hd_calc_sector_loc(&loc, offset, size, meta);
    if (loc.size == 0) {
        rwq_deallocate_pos(&rw_queue, wait_idx);
        return 0;
    }

    _debug_printf(
        "[wait_idx=%u] lba=%u, sector count=%u\n",
        wait_idx,
        loc.low_sector_lba,
        loc.sector_count
    );

    uint16_t* buffer = kmalloc_flags(
        loc.sector_size * loc.sector_count,
        KMALLOC_ZERO
    );

    if (offset != loc.lowest_sector_byte || size % loc.sector_size != 0) {
        _debug_printf(
            "[wait_idx=%u] not aligned to sectors - reading wait started\n",
            wait_idx
        );
        ata_read(
            meta->disk_id,
            loc.low_sector_lba,
            loc.sector_count,
            buffer,
            rw_ready,
            &wakeup_data
        );
        wait_on_condition(meta->wait_obj, rw_wait_for_ready, &wakeup_data);

        _debug_printf("[wait_idx=%u] waiting completed\n", wait_idx);
        rwq_reset_to_not_ready(&rw_queue, wait_idx);
    }

    size_t offset_in_buffer = offset - loc.lowest_sector_byte;
    memcpy(((uint8_t*)buffer) + offset_in_buffer, ptr, loc.size);

    ata_write(
        meta->disk_id,
        loc.low_sector_lba,
        loc.sector_count,
        buffer,
        rw_ready,
        &wakeup_data
    );

    _debug_printf("[wait_idx=%u] start for write\n", wait_idx);

    wait_on_condition(meta->wait_obj, rw_wait_for_ready, &wakeup_data);

    _debug_printf("[wait_idx=%u] waiting completed\n", wait_idx);

    rwq_deallocate_pos(&rw_queue, wait_idx);

    kfree(buffer);
    return loc.size;
}

// Disk-level nodes (hda, hdb, sr0) — permanent, freed on unmount.
static vfs_node_t** disk_nodes = NULL;
static size_t disk_nodes_count = 0;

// Shared wait_obj for blocking MBR reads done during readdir/finddir.
static wait_obj_t* partition_wait_obj = NULL;

// Per-PIO-disk partition cache, rebuilt on each readdir pass.
typedef struct {
    uint8_t disk_id;
    char drive_letter;
    ata_partition_t parts[4];
    uint8_t count;
} hd_part_cache_entry_t;

#define MAX_PIO_DISKS 4
static hd_part_cache_entry_t part_cache[MAX_PIO_DISKS];
static size_t part_cache_pio_count = 0;

// on_release for dynamically-allocated partition nodes.
static void partition_node_release(vfs_node_t* node)
{
    hd_metadata_t* meta = node->metadata;
    if (meta != NULL) {
        if (meta->wait_obj != NULL) {
            wait_deallocate(meta->wait_obj);
        }
        kfree(meta);
    }
    kfree(node);
}

// Blocking read of sector 0 from disk_id into mbr_buf (must be sector-sized).
// Uses partition_wait_obj and the shared rw_queue.
static void read_mbr_sync(uint8_t disk_id, uint16_t* mbr_buf)
{
    hd_metadata_t fake_meta = {
        .disk_id = disk_id,
        .is_partition = false,
        .wait_obj = partition_wait_obj,
    };
    size_t wait_idx = rwq_allocate_pos(&rw_queue);
    hd_wakeup_data_t wd = {
        .wait_idx = wait_idx,
        .hd_metadata = &fake_meta,
    };
    ata_read(disk_id, 0, 1, mbr_buf, rw_ready, &wd);
    wait_on_condition(partition_wait_obj, rw_wait_for_ready, &wd);
    rwq_deallocate_pos(&rw_queue, wait_idx);
}

// Refresh part_cache by reading current MBR from every PIO disk.
static void refresh_partition_cache(void)
{
    part_cache_pio_count = 0;
    uint8_t avail[5];
    ata_get_available_drives(avail);
    char drive_letter = 'a';

    for (uint8_t* d = avail; *d != ATA_DRIVE_NONE; d++) {
        ata_disk_info_t di;
        ata_load_disk_info(*d, &di);

        if (di.type != PIO) {
            continue;
        }
        if (part_cache_pio_count >= MAX_PIO_DISKS) {
            break;
        }

        uint16_t* mbr_buf = kmalloc_flags(di.sector_size, KMALLOC_ZERO);
        read_mbr_sync(*d, mbr_buf);

        hd_part_cache_entry_t* entry = &part_cache[part_cache_pio_count];
        entry->disk_id = *d;
        entry->drive_letter = drive_letter;
        entry->count = ata_parse_mbr_partitions(
            (uint8_t*)mbr_buf,
            entry->parts
        );
        part_cache_pio_count++;

        kfree(mbr_buf);
        drive_letter++;
    }
}

// Called by device.c readdir for indices past the static device list.
// part_index == 0 triggers a fresh MBR read for all PIO disks.
bool device_hd_readdir_partition(size_t part_index, struct dirent* out)
{
    if (partition_wait_obj == NULL) {
        return false;
    }

    if (part_index == 0) {
        refresh_partition_cache();
    }

    size_t idx = 0;
    for (size_t d = 0; d < part_cache_pio_count; d++) {
        for (size_t p = 0; p < part_cache[d].count; p++) {
            if (idx == part_index) {
                out->name[0] = 'h';
                out->name[1] = 'd';
                out->name[2] = part_cache[d].drive_letter;
                out->name[3] = '1' + p;
                out->name[4] = '\0';
                out->ino = (uint32_t)(part_cache[d].disk_id * 4 + p + 1);
                return true;
            }
            idx++;
        }
    }
    return false;
}

// Called by device.c finddir for names not matching static devices.
// Reads current MBR on demand. Returns a dynamically-allocated node with
// on_release set so it is freed when the last file handle is closed.
vfs_node_t* device_hd_finddir_partition(const char* name)
{
    if (partition_wait_obj == NULL) {
        return NULL;
    }

    // Accept names of the form "hd[a-z][1-9]"
    if (name[0] != 'h' || name[1] != 'd'
        || name[2] < 'a' || name[2] > 'z'
        || name[3] < '1' || name[3] > '9'
        || name[4] != '\0') {
        return NULL;
    }
    char drive_letter = name[2];
    uint8_t part_num = (uint8_t)(name[3] - '1'); // 0-based

    uint8_t avail[5];
    ata_get_available_drives(avail);
    char dl = 'a';

    for (uint8_t* d = avail; *d != ATA_DRIVE_NONE; d++) {
        ata_disk_info_t di;
        ata_load_disk_info(*d, &di);

        if (di.type != PIO) {
            continue;
        }

        if (dl != drive_letter) {
            dl++;
            continue;
        }

        uint16_t* mbr_buf = kmalloc_flags(di.sector_size, KMALLOC_ZERO);
        read_mbr_sync(*d, mbr_buf);

        ata_partition_t parts[4];
        uint8_t count = ata_parse_mbr_partitions((uint8_t*)mbr_buf, parts);
        kfree(mbr_buf);

        if (part_num >= count) {
            return NULL;
        }

        ata_partition_t* part = &parts[part_num];

        vfs_node_t* node = kmalloc_flags(sizeof(vfs_node_t), KMALLOC_ZERO);
        vfs_populate_node(node, (char*)name, VFS_TYPE_BLOCK_DEVICE);
        node->read_node = read;
        node->write_node = write;
        node->length = (uint64_t)part->sectors_count * di.sector_size;
        node->on_release = partition_node_release;

        hd_metadata_t* metadata = kmalloc_flags(
            sizeof(hd_metadata_t),
            KMALLOC_ZERO
        );
        metadata->disk_id = *d;
        metadata->is_partition = true;
        metadata->part_lba_start = part->lba_start;
        metadata->part_sectors_count = part->sectors_count;
        metadata->wait_obj = wait_allocate_queue();
        node->metadata = metadata;

        return node;
    }

    return NULL;
}

vfs_node_t** device_hd_mount()
{
    if (disk_nodes != NULL) {
        return disk_nodes;
    }

    partition_wait_obj = wait_allocate_queue();

    _debug_puts("Mounting HD disk files to VFS");
    uint8_t ata_drives[5];
    ata_get_available_drives(ata_drives);
    uint8_t* ata_drive_ptr = ata_drives;
    char drive_name[] = "hda";
    char drive_letter = 'a';
    char sr_name[] = "sr0";
    char sr_index = '0';

    disk_nodes = kmalloc_flags(sizeof(vfs_node_t*), KMALLOC_ZERO);
    disk_nodes[0] = NULL;

    while (*ata_drive_ptr != ATA_DRIVE_NONE) {
        ata_disk_info_t disk_info;
        ata_load_disk_info(*ata_drive_ptr, &disk_info);
        size_t sector_size = disk_info.sector_size;

        if (disk_info.type == ATAPI) {
            sr_name[2] = sr_index;
            _debug_printf("ATAPI drive found, mounting to /dev/%s\n", sr_name);

            vfs_node_t* node = kmalloc_flags(sizeof(vfs_node_t), KMALLOC_ZERO);
            vfs_populate_node(node, sr_name, VFS_TYPE_BLOCK_DEVICE);
            node->read_node = read;
            node->write_node = NULL;
            node->length = (uint64_t)disk_info.sectors_count * sector_size;
            hd_metadata_t* metadata = kmalloc_flags(
                sizeof(hd_metadata_t),
                KMALLOC_ZERO
            );
            node->metadata = metadata;
            metadata->disk_id = *ata_drive_ptr;
            metadata->wait_obj = wait_allocate_queue();
            metadata->is_partition = false;

            disk_nodes_count++;
            disk_nodes = krealloc(
                disk_nodes,
                sizeof(vfs_node_t*) * (disk_nodes_count + 1)
            );
            disk_nodes[disk_nodes_count - 1] = node;
            disk_nodes[disk_nodes_count] = NULL;

            sr_index++;
            ata_drive_ptr++;
            continue;
        }

        if (disk_info.type != PIO) {
            ata_drive_ptr++;
            continue;
        }

        drive_name[2] = drive_letter;
        _debug_printf("PIO drive found, mounting to /dev/%s\n", drive_name);

        vfs_node_t* node = kmalloc_flags(sizeof(vfs_node_t), KMALLOC_ZERO);
        vfs_populate_node(node, drive_name, VFS_TYPE_BLOCK_DEVICE);
        node->read_node = read;
        node->write_node = write;
        node->length = (uint64_t)disk_info.sectors_count * sector_size;
        hd_metadata_t* metadata = kmalloc_flags(
            sizeof(hd_metadata_t),
            KMALLOC_ZERO
        );
        node->metadata = metadata;
        metadata->disk_id = *ata_drive_ptr;
        metadata->wait_obj = wait_allocate_queue();
        metadata->is_partition = false;

        disk_nodes_count++;
        disk_nodes = krealloc(
            disk_nodes,
            sizeof(vfs_node_t*) * (disk_nodes_count + 1)
        );
        disk_nodes[disk_nodes_count - 1] = node;
        disk_nodes[disk_nodes_count] = NULL;

        drive_letter++;
        ata_drive_ptr++;
    }

    return disk_nodes;
}

void device_hd_unmount()
{
    if (disk_nodes == NULL) {
        return;
    }
    vfs_node_t** ptr = disk_nodes;
    while (*ptr != NULL) {
        vfs_node_t* node = *ptr;
        hd_metadata_t* metadata = node->metadata;
        if (metadata != NULL) {
            if (metadata->wait_obj != NULL) {
                wait_deallocate(metadata->wait_obj);
            }
            kfree(metadata);
        }
        kfree(node);
        ptr++;
    }
    kfree(disk_nodes);
    disk_nodes_count = 0;
    disk_nodes = NULL;

    if (partition_wait_obj != NULL) {
        wait_deallocate(partition_wait_obj);
        partition_wait_obj = NULL;
    }
}
