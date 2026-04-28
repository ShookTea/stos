#include <stdint.h>
#include <stdbool.h>
#include "kernel/memory/kmalloc.h"
#include "kernel/vfs/vfs.h"
#include "./ext2.h"

bool ext2_ensure_dir_cache(vfs_node_t* node)
{
    ext2_inode_metadata_t* meta = node->metadata;
    if (meta->dir_cache != NULL) return true;

    vfs_file_t* file = vfs_open(meta->device_file, VFS_MODE_READONLY);
    if (file == NULL) return false;

    if (meta->cached_inode == NULL) {
        meta->cached_inode = ext2_read_inode(file, meta, node->inode);
    }
    ext2_inode_t* inode = meta->cached_inode;

    uint32_t bs = meta->block_size;
    uint32_t block_count = 0;
    for (int i = 0; i < 12; i++) {
        if (inode->direct_block_pointers[i] == 0) break;
        block_count++;
    }

    // Clamp to what size_lo implies
    if (inode->size_lo > 0) {
        uint32_t size_blocks = (inode->size_lo + bs - 1) / bs;
        if (block_count > size_blocks) block_count = size_blocks;
    } else {
        block_count = 0;
    }

    meta->dir_cache_size = block_count * bs;
    meta->dir_cache = kmalloc_flags(
        block_count > 0 ? meta->dir_cache_size : 1,
        KMALLOC_ZERO
    );

    for (uint32_t i = 0; i < block_count; i++) {
        uint64_t byte_off = (uint64_t)inode->direct_block_pointers[i] * bs;
        vfs_seek(file, byte_off);
        vfs_read(file, bs, meta->dir_cache + i * bs);
    }

    vfs_close(file);
    return true;
}

ext2_inode_t* ext2_read_inode(
    vfs_file_t* file,
    ext2_inode_metadata_t* meta,
    uint32_t inode_num
) {
    uint32_t bs = meta->block_size;
    uint32_t block_group = (inode_num - 1) / meta->inodes_per_group;
    uint32_t index_in_group = (inode_num - 1) % meta->inodes_per_group;

    uint32_t gdt_start = (bs == 1024 ? 2 : 1) * bs;
    uint32_t gdt_seek = gdt_start + block_group * sizeof(ext2_gdt_t);
    vfs_seek(file, gdt_seek);
    ext2_gdt_t* gdt = kmalloc_flags(sizeof(ext2_gdt_t), KMALLOC_ZERO);
    vfs_read(file, sizeof(ext2_gdt_t), gdt);
    uint32_t inode_table_byte = gdt->inode_table_start_addr * bs;
    kfree(gdt);

    uint32_t inode_byte = inode_table_byte + index_in_group * meta->inode_size;
    ext2_inode_t* inode = kmalloc_flags(sizeof(ext2_inode_t), KMALLOC_ZERO);
    vfs_seek(file, inode_byte);
    vfs_read(file, sizeof(ext2_inode_t), inode);
    return inode;
}
