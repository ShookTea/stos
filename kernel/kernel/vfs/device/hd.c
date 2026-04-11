#include "../device.h"
#include "kernel/debug.h"
#include "kernel/drivers/ata.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/task/wait.h"
#include "kernel/vfs/vfs.h"
#include "stdlib.h"
#include <string.h>

#define _debug_puts(...) debug_puts_c("VFS/dev/hd", __VA_ARGS__)
#define _debug_printf(...) debug_printf_c("VFS/dev/hd", __VA_ARGS__)

#define SECTOR_SIZE 512

static inline size_t sector_align_down(size_t addr)
{
    return addr & ~(SECTOR_SIZE - 1);
}

static inline size_t sector_align_up(size_t addr)
{
    return (addr + SECTOR_SIZE - 1) & ~(SECTOR_SIZE - 1);
}

typedef struct {
    // one of ATA_DRIVE_ values
    uint8_t disk_id;
    // Is it representing a single partition, or a whole disk?
    bool is_partition;
    // Partition ID - ignored if is_partition=false
    uint8_t partition_id;
    // Tasks queue
    wait_obj_t* wait_obj;
} hd_metadata_t;

typedef struct {
    hd_metadata_t* hd_metadata;
    size_t wait_idx;
} hd_wakeup_data_t;

/**
 * Set of values for the is_ready function.
 * 0 = free to use
 * 1 = not ready
 * 2 = ready
 */
static uint8_t* rw_wait_map = NULL;
static size_t rw_wait_map_count = 0;
static size_t rw_wait_map_capacity = 0;

/**
 * Creates a new entry in rw_wait_map in "not ready" state and returns ID of
 * the entry.
 */
static size_t allocate_rw_wait_pos()
{
    if (rw_wait_map_capacity == rw_wait_map_count) {
        rw_wait_map_capacity += 10;
        rw_wait_map = krealloc(
            rw_wait_map,
            sizeof(uint8_t) * rw_wait_map_capacity
        );
        memset(rw_wait_map + rw_wait_map_count, 0, 10);
        rw_wait_map[rw_wait_map_count] = 1;
        rw_wait_map_count++;
        return rw_wait_map_count - 1;
    }

    for (size_t i = 0; i < rw_wait_map_capacity; i++) {
        if (rw_wait_map[i] == 0) {
            rw_wait_map[i] = 1;
            return i;
        }
    }
    _debug_puts("ERR: unexpected situation at allocate_rw_wait_pos");
    abort();
}

// Mark RW read from the pointer as ready
static void rw_ready(void* ptr)
{
    hd_wakeup_data_t* data = ptr;
    size_t id = data->wait_idx;
    if (id >= rw_wait_map_capacity) {
        _debug_puts("ERR: id >= rw_wait_map_capacity");
        abort();
    }
    rw_wait_map[id] = 2;

    // Wakeup process in the queue
    wait_wake_up(data->hd_metadata->wait_obj);
}

static bool rw_wait_for_ready(void* ptr)
{
    hd_wakeup_data_t* data = ptr;
    size_t id = data->wait_idx;
    if (id >= rw_wait_map_capacity) {
        _debug_puts("ERR: id >= rw_wait_map_capacity");
        abort();
    }
    return rw_wait_map[id] == 2;
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

    hd_metadata_t* meta = file->node->metadata;
    if (meta->is_partition) {
        // TODO: handle reading from specific partition
        return 0;
    }

    ata_select_drive(meta->disk_id);
    size_t wait_idx = allocate_rw_wait_pos();

    hd_wakeup_data_t wakeup_data;
    wakeup_data.wait_idx = wait_idx;
    wakeup_data.hd_metadata = meta;

    _debug_printf(
        "Received READ call for offset=%u size=%u, wait_idx=%u\n",
        offset,
        size,
        wait_idx
    );

    // Align offset to sector size
    size_t lowest_sector_byte = sector_align_down(offset);
    size_t highest_sector_byte = sector_align_up(offset + size);
    size_t low_sector_lba = lowest_sector_byte / SECTOR_SIZE;
    size_t high_sector_lba = highest_sector_byte / SECTOR_SIZE;
    size_t sector_count = high_sector_lba - low_sector_lba;

    _debug_printf(
        "[wait_idx=%u] lba=%u, sector count=%u\n",
        wait_idx,
        low_sector_lba,
        sector_count
    );

    // Allocate buffer for sector data
    uint16_t* buffer = kmalloc_flags(
        sizeof(uint16_t) * 256 * sector_count,
        KMALLOC_ZERO
    );

    // Run read command
    ata_read(
        low_sector_lba,
        sector_count,
        buffer,
        rw_ready,
        &wakeup_data
    );

    _debug_printf("[wait_idx=%u] start waiting\n", wait_idx);

    // Wait for reading to be completed
    wait_on_condition(meta->wait_obj, rw_wait_for_ready, &wakeup_data);

    _debug_printf("[wait_idx=%u] waiting completed\n", wait_idx);

    // Reading was completed - copy data to output pointer
    size_t offset_in_buffer = offset - lowest_sector_byte;
    memcpy(ptr, ((uint8_t*)buffer) + offset_in_buffer, size);

    // Mark rw_map entry as free to use
    rw_wait_map[wait_idx] = 0;

    kfree(buffer);
    return size;
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

    hd_metadata_t* meta = file->node->metadata;
    if (meta->is_partition) {
        // TODO: handle reading from specific partition
        return 0;
    }

    ata_select_drive(meta->disk_id);
    size_t wait_idx = allocate_rw_wait_pos();

    hd_wakeup_data_t wakeup_data;
    wakeup_data.wait_idx = wait_idx;
    wakeup_data.hd_metadata = meta;

    _debug_printf(
        "Received WRITE call for offset=%u size=%u, wait_idx=%u\n",
        offset,
        size,
        wait_idx
    );

    // Align offset to sector size
    size_t lowest_sector_byte = sector_align_down(offset);
    size_t highest_sector_byte = sector_align_up(offset + size);
    size_t low_sector_lba = lowest_sector_byte / SECTOR_SIZE;
    size_t high_sector_lba = highest_sector_byte / SECTOR_SIZE;
    size_t sector_count = high_sector_lba - low_sector_lba;

    _debug_printf(
        "[wait_idx=%u] lba=%u, sector count=%u\n",
        wait_idx,
        low_sector_lba,
        sector_count
    );

    // Allocate buffer for sector data
    uint16_t* buffer = kmalloc_flags(
        sizeof(uint16_t) * 256 * sector_count,
        KMALLOC_ZERO
    );

    // If original offset and size was not aligned to sectors, we need to first
    // load data, so we won't overwrite parts of sectors that are not updated
    // by the caller.
    if (offset != lowest_sector_byte || size % SECTOR_SIZE != 0) {
        _debug_printf(
            "[wait_idx=%u] not aligned to sectors - reading wait started\n",
            wait_idx
        );
        // Run read command
        ata_read(
            low_sector_lba,
            sector_count,
            buffer,
            rw_ready,
            &wakeup_data
        );
        // Wait for reading to be completed
        wait_on_condition(meta->wait_obj, rw_wait_for_ready, &wakeup_data);

        _debug_printf("[wait_idx=%u] waiting completed\n", wait_idx);
        // Re-set wait map back to "waiting"
        rw_wait_map[wait_idx] = 1;
    }

    // Copy data from pointer to sectors
    size_t offset_in_buffer = offset - lowest_sector_byte;
    memcpy(((uint8_t*)buffer) + offset_in_buffer, ptr, size);

    // Run write command
    ata_write(
        low_sector_lba,
        sector_count,
        buffer,
        rw_ready,
        &wakeup_data
    );

    _debug_printf("[wait_idx=%u] start for write\n", wait_idx);

    // Wait for reading to be completed
    wait_on_condition(meta->wait_obj, rw_wait_for_ready, &wakeup_data);

    _debug_printf("[wait_idx=%u] waiting completed\n", wait_idx);

    // Mark rw_map entry as free to use
    rw_wait_map[wait_idx] = 0;

    kfree(buffer);
    return size;
}

static vfs_node_t** nodes = NULL;

vfs_node_t** device_hd_mount()
{
    if (nodes != NULL) {
        return nodes;
    }

    // 4 pointers + 1 terminating NULL pointer
    nodes = kmalloc_flags(sizeof(vfs_node_t*) * 5, KMALLOC_ZERO);

    _debug_puts("Mounting HD files to VFS");
    uint8_t ata_drives[5];
    ata_get_available_drives(ata_drives);
    uint8_t* ata_drive_ptr = ata_drives;
    char drive_name[] = "hda";
    char drive_letter = 'a';
    size_t pointer_index = 0;
    while (*ata_drive_ptr != 0) {
        drive_name[2] = drive_letter;
        _debug_printf(
            "drive with type %d found, mounting to /dev/%s\n",
            *ata_drive_ptr,
            drive_name
        );
        ata_select_drive(*ata_drive_ptr);

        vfs_node_t* node = kmalloc_flags(sizeof(vfs_node_t), KMALLOC_ZERO);
        vfs_populate_node(node, drive_name, VFS_TYPE_BLOCK_DEVICE);
        node->read_node = read;
        node->write_node = write;
        node->length = (uint64_t)ata_lba28_sectors_count() * SECTOR_SIZE;
        _debug_printf("size set to %llu\n", node->length);
        hd_metadata_t* metadata = kmalloc_flags(
            sizeof(hd_metadata_t),
            KMALLOC_ZERO
        );
        node->metadata = metadata;
        metadata->disk_id = *ata_drive_ptr;
        metadata->wait_obj = wait_allocate_queue();
        metadata->is_partition = false;
        nodes[pointer_index] = node;

        // TODO: load partitions to /dev/hda1, /dev/hda2 etc.

        drive_letter++;
        ata_drive_ptr++;
        pointer_index++;
    }

    return nodes;
}

void device_hd_unmount()
{
    if (nodes == NULL) {
        return;
    }
    vfs_node_t** ptr = nodes;
    while (*ptr != NULL) {
        vfs_node_t* node = *ptr;
        hd_metadata_t* metadata = node->metadata;
        if (metadata != NULL) {
            if (metadata->wait_obj != NULL) {
                kfree(metadata->wait_obj);
            }
            kfree(metadata);
        }
        kfree(node);
        ptr++;
    }
    kfree(nodes);
    nodes = NULL;
}
