#include <stdint.h>
#include "kernel/memory/kmalloc.h"
#include "kernel/vfs/vfs.h"
#include "./ext2.h"

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
