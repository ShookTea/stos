#ifndef INCLUDE_KERNEL_SRC_VFS_MOUNT_EXT2_H
#define INCLUDE_KERNEL_SRC_VFS_MOUNT_EXT2_H

#include <stdint.h>
#include "kernel/vfs/vfs.h"

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
    /* Extended fields (major_version >= 1) */
    uint32_t first_unreserved_inode;
    uint16_t inode_size;
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
 * Directory entry as stored on disk
 */
typedef struct __attribute__((packed)) {
    uint32_t inode;
    uint16_t rec_len;
    uint8_t  name_len;
    uint8_t  file_type;
    char     name[];
} ext2_dir_entry_t;

/**
 * Definition of inode
 */
typedef struct __attribute__((packed)) {
    uint16_t type_and_permissions;
    uint16_t user_id;
    uint32_t size_lo;
    uint32_t last_access_time;
    uint32_t creation_time;
    uint32_t last_modification_time;
    uint32_t deletion_time;
    uint16_t group_id;
    uint16_t hard_links_count;
    uint32_t disk_sectors_count;
    uint32_t flags;
    uint32_t os_specific_val_1;
    uint32_t direct_block_pointers[12];
    uint32_t singly_indirect_block_pointer;
    uint32_t doubly_indirect_block_pointer;
    uint32_t triply_indirect_block_pointer;
    uint32_t generation_number;
    uint32_t __reserved1;
    uint32_t __reserved2; // potentially size_hi
    uint32_t block_address_of_fragment;
    uint32_t os_specific_val_2[3];
} ext2_inode_t;

/**
 * Metadata added to inodes mounted in ext2
 */
typedef struct {
    dentry_t* device_file;
    uint32_t inodes_per_group;
    uint32_t block_size;
    uint16_t inode_size;
    ext2_inode_t* cached_inode;
    uint8_t* dir_cache;
    uint32_t dir_cache_size;
} ext2_inode_metadata_t;

bool ext2_readdir(vfs_node_t* node, size_t index, struct dirent* out);
vfs_node_t* ext2_finddir(vfs_node_t* node, char* name);
void ext2_on_release(vfs_node_t* node);

ext2_inode_t* ext2_read_inode(
    vfs_file_t* file,
    ext2_inode_metadata_t* meta,
    uint32_t inode_num
);

bool ext2_ensure_dir_cache(vfs_node_t* node);

inline uint8_t ext2_type_to_vfs(uint16_t type_and_permissions)
{
    switch (type_and_permissions & 0xF000) {
        case 0x4000: return VFS_TYPE_DIRECTORY;
        case 0x8000: return VFS_TYPE_FILE;
        case 0xA000: return VFS_TYPE_SYMLINK;
        case 0x6000: return VFS_TYPE_BLOCK_DEVICE;
        case 0x2000: return VFS_TYPE_CHARACTER_DEVICE;
        default: return VFS_TYPE_FILE;
    }
}

#endif
