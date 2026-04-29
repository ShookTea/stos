#include <stdint.h>
#include "kernel/vfs/vfs.h"
#include "./ext2.h"

void ext2_write_inode(
    dentry_t* device,
    uint32_t inode_num,
    uint32_t block_size,
    uint16_t inode_size,
    uint32_t inodes_per_group,
    ext2_inode_t* inode
) {
    vfs_file_t* file = vfs_open(device, VFS_MODE_READWRITE);
    if (file == NULL) {
        return;
    }

    uint32_t block_group = (inode_num - 1) / inodes_per_group;
    uint32_t index_in_group = (inode_num - 1) % inodes_per_group;
    uint32_t gdt_start = (block_size == 1024 ? 2u : 1u) * block_size;

    vfs_seek(file, gdt_start + (uint64_t)block_group * sizeof(ext2_gdt_t));
    ext2_gdt_t gdt;
    vfs_read(file, sizeof(ext2_gdt_t), &gdt);

    uint64_t inode_byte =
        (uint64_t)gdt.inode_table_start_addr * block_size
        + (uint64_t)index_in_group * inode_size;
    vfs_seek(file, inode_byte);
    vfs_write(file, sizeof(ext2_inode_t), inode);

    vfs_close(file);
}
