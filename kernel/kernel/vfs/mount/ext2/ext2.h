#ifndef INCLUDE_KERNEL_SRC_VFS_MOUNT_EXT2_H
#define INCLUDE_KERNEL_SRC_VFS_MOUNT_EXT2_H

#include <stdint.h>

typedef struct __attribute__((packed)) {
    uint32_t total_inodes;
    uint32_t total_blocks;
    /** Number of blocks reserved for superuser */
    uint32_t total_su_reserved_blocks;
    /** Total number of unallocated blocks */
    uint32_t total_unalloc_blocks;
    /** Total number of unallocated inodes */
    uint32_t total_unalloc_inodes;
    /** Block number of the superblock */
    uint32_t sb_blocknum;
    /** The number to shift 1024 to the left to obtain block size */
    uint32_t block_size;
    /** The number to shift 1024 to the left to obtain fragment size */
    uint32_t fragment_size;
    /** Number of blocks in each block group */
    uint32_t blocks_per_group;
    /** Number of fragments in each block group */
    uint32_t fragm_per_group;
    /** Number of inodes in each block group */
    uint32_t inodes_per_group;
    uint32_t last_mount_time;
    uint32_t last_written_time;
    /** Number of times volume has been mounted since last consistency check */
    uint16_t mounts_since_last_fsck;
    /** Number of allowed mounts before fsck must be done */
    uint16_t mounts_before_fsck;
    /** 0xEF53 - signature of ext2 */
    uint16_t signature;
    uint16_t file_system_state;
    uint16_t error_handling_method;
    uint16_t minor_version;
    uint32_t last_fsck_time;
    uint32_t interval_between_forced_fsck;
    /** Operating system Id where the volume was created */
    uint32_t os_creator_id;
    uint32_t major_version;
    /** User ID that can use reserved blocks */
    uint16_t su_uid;
    /** Group ID that can use reserved blocks */
    uint16_t su_gid;
} ext2_superblock_t;

/**
 * Group descriptor table definition
 */
typedef struct __attribute__((packed)) {
    uint32_t block_usage_bitmap_addr;
    uint32_t inode_usage_bitmap_addr;
    uint32_t inode_table_start_addr;
    uint16_t unalloc_blocks_count;
    uint16_t unalloc_inodes_count;
    uint16_t directories_count;
    uint8_t __unused[14];
} ext2_gdt_t;

/**
 * Metadata added to inodes mounted in ext2
 */
typedef struct {
    uint32_t inodes_per_group;
} ext2_inode_metadata_t;

#endif
