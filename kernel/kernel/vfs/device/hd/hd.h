#ifndef INCLUDE_KERNEL_SRC_VFS_DEVICE_HD_H
#define INCLUDE_KERNEL_SRC_VFS_DEVICE_HD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "kernel/task/wait.h"
#include "../../rw_queue/rw_queue.h"
#include "kernel/vfs/vfs.h"

typedef struct {
    // one of ATA_DRIVE_ values
    uint8_t disk_id;
    // Is it representing a single partition, or a whole disk?
    bool is_partition;
    // For partitions: LBA and size stored directly (no boot-time cache needed)
    uint32_t part_lba_start;
    uint32_t part_sectors_count;
    // Tasks queue
    wait_obj_t* wait_obj;
} hd_metadata_t;

typedef struct {
    hd_metadata_t* hd_metadata;
    size_t wait_idx;
} hd_wakeup_data_t;

typedef struct {
    size_t size;
    size_t low_sector_lba;
    size_t sector_count;
    size_t lowest_sector_byte;
    size_t sector_size;
} hd_sector_location_t;

// Shared wait_obj for blocking MBR reads done during readdir/finddir.
extern wait_obj_t* hd_partition_wait_obj;

size_t hd_write(vfs_file_t* file, size_t offset, size_t size, const void* ptr);
size_t hd_read(vfs_file_t* file, size_t offset, size_t size,void* ptr);

/**
 * Calculate sector location based on given offset and size, and store it in
 * given `loc` pointer.
 */
void hd_calc_sector_loc(
    hd_sector_location_t* loc,
    size_t offset,
    size_t size,
    hd_metadata_t* meta
);

/**
 * Blocking read of sector 0 from disk_id into mbr_buf (must be sector-sized).
 */
void hd_read_mbr_sync(uint8_t disk_id, uint16_t* mbr_buf);

extern rw_queue_t hd_rw_queue;
void hd_rw_ready(void* ptr);
bool hd_rw_wait_for_ready(void* ptr);

#endif
