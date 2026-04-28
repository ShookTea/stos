#include <stddef.h>
#include <stdint.h>
#include "./ext2.h"
#include "kernel/vfs/vfs.h"

static uint32_t read_block_ptr(
    vfs_file_t* file,
    uint32_t block_num,
    size_t index,
    size_t block_size
) {
    uint32_t ptr = 0;
    vfs_seek(file, (uint64_t)block_num * block_size + index * sizeof(uint32_t));
    vfs_read(file, sizeof(uint32_t), &ptr);
    return ptr;
}

size_t ext2_block_id_to_addr(
    dentry_t* device,
    ext2_inode_t* inode,
    size_t block_id,
    size_t block_size
) {
    if ((uint64_t)block_id * block_size >= inode->size_lo) {
        return 0;
    }

    if (block_id < 12) {
        return (size_t)inode->direct_block_pointers[block_id] * block_size;
    }
    block_id -= 12;

    size_t ppb = block_size / sizeof(uint32_t);

    vfs_file_t* file = vfs_open(device, VFS_MODE_READONLY);
    if (file == NULL) return 0;

    uint32_t block_num = 0;

    if (block_id < ppb) {
        // Singly indirect
        block_num = read_block_ptr(file, inode->singly_indirect_block_pointer, block_id, block_size);
    } else {
        block_id -= ppb;
        if (block_id < ppb * ppb) {
            // Doubly indirect
            block_num = read_block_ptr(file, inode->doubly_indirect_block_pointer, block_id / ppb, block_size);
            block_num = read_block_ptr(file, block_num, block_id % ppb, block_size);
        } else {
            block_id -= ppb * ppb;
            // Triply indirect
            block_num = read_block_ptr(file, inode->triply_indirect_block_pointer, block_id / (ppb * ppb), block_size);
            block_num = read_block_ptr(file, block_num, (block_id / ppb) % ppb, block_size);
            block_num = read_block_ptr(file, block_num, block_id % ppb, block_size);
        }
    }

    vfs_close(file);
    return (size_t)block_num * block_size;
}
