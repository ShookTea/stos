#include "kernel/debug.h"
#include "kernel/drivers/ata.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/task/wait.h"
#include "kernel/vfs/vfs.h"
#include "stdlib.h"
#include <stddef.h>
#include <string.h>
#include "../../device.h"
#include "./hd.h"

#define _debug_puts(...) debug_puts_c("VFS/dev/hd", __VA_ARGS__)
#define _debug_printf(...) debug_printf_c("VFS/dev/hd", __VA_ARGS__)

// Disk-level nodes (hda, hdb, sr0) — permanent, freed on unmount.
static vfs_node_t** disk_nodes = NULL;
static size_t disk_nodes_count = 0;

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
        hd_read_mbr_sync(*d, mbr_buf);

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
    if (hd_partition_wait_obj == NULL) {
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

vfs_node_t** device_hd_mount()
{
    if (disk_nodes != NULL) {
        return disk_nodes;
    }

    hd_partition_wait_obj = wait_allocate_queue();

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
            node->read_node = hd_read;
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
        node->read_node = hd_read;
        node->write_node = hd_write;
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

    if (hd_partition_wait_obj != NULL) {
        wait_deallocate(hd_partition_wait_obj);
        hd_partition_wait_obj = NULL;
    }
}
