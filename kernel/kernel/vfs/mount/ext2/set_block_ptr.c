#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "kernel/memory/kmalloc.h"
#include "kernel/vfs/vfs.h"
#include "./ext2.h"

/*
 * Ensures *ptr_field points to an allocated indirect block.
 * If zero, allocates a new block via ext2_alloc_block, zeros it through
 * the provided file handle, and stores the new block number in *ptr_field.
 * Returns the block number, or 0 on failure.
 */
static uint32_t ensure_indirect(
    vfs_file_t* file,
    dentry_t* device,
    uint32_t* ptr_field,
    uint32_t block_size
) {
    if (*ptr_field != 0) {
        return *ptr_field;
    }

    uint32_t new_block = ext2_alloc_block(device, block_size);
    if (new_block == 0) {
        return 0;
    }

    uint8_t* zeros = kmalloc_flags(block_size, KMALLOC_ZERO);
    vfs_seek(file, (uint64_t)new_block * block_size);
    vfs_write(file, block_size, zeros);
    kfree(zeros);

    *ptr_field = new_block;
    return new_block;
}

bool ext2_set_block_id(
    dentry_t* device,
    ext2_inode_t* inode,
    size_t block_id,
    size_t block_size,
    uint32_t new_block_num
) {
    if (block_id < 12) {
        inode->direct_block_pointers[block_id] = new_block_num;
        return true;
    }
    block_id -= 12;

    size_t ppb = block_size / sizeof(uint32_t);

    vfs_file_t* file = vfs_open(device, VFS_MODE_READWRITE);
    if (file == NULL) return false;

    bool ok = false;

    if (block_id < ppb) {
        // Singly indirect
        uint32_t sib = inode->singly_indirect_block_pointer;
        uint32_t l1 = ensure_indirect(file, device, &sib, block_size);
        inode->singly_indirect_block_pointer = sib;

        if (l1 != 0) {
            vfs_seek(
                file,
                (uint64_t)l1 * block_size + block_id * sizeof(uint32_t)
            );
            vfs_write(file, sizeof(uint32_t), &new_block_num);
            ok = true;
        }
    } else {
        block_id -= ppb;

        if (block_id < ppb * ppb) {
            // Doubly indirect
            uint32_t dib = inode->doubly_indirect_block_pointer;
            uint32_t l1 = ensure_indirect(file, device, &dib, block_size);
            inode->doubly_indirect_block_pointer = dib;
            if (l1 != 0) {
                uint32_t l2 = 0;
                vfs_seek(
                    file,
                    (uint64_t)l1 * block_size
                        + (block_id / ppb) * sizeof(uint32_t)
                );
                vfs_read(file, sizeof(uint32_t), &l2);

                if (l2 == 0) {
                    l2 = ext2_alloc_block(device, block_size);
                    if (l2 != 0) {
                        uint8_t* zeros = kmalloc_flags(
                            block_size,
                            KMALLOC_ZERO
                        );
                        vfs_seek(file, (uint64_t)l2 * block_size);
                        vfs_write(file, block_size, zeros);
                        kfree(zeros);
                        vfs_seek(
                            file,
                            (uint64_t)l1 * block_size
                                + (block_id / ppb) * sizeof(uint32_t)
                        );
                        vfs_write(file, sizeof(uint32_t), &l2);
                    }
                }
                if (l2 != 0) {
                    vfs_seek(
                        file,
                        (uint64_t)l2 * block_size
                            + (block_id % ppb) * sizeof(uint32_t)
                    );
                    vfs_write(file, sizeof(uint32_t), &new_block_num);
                    ok = true;
                }
            }
        } else {
            block_id -= ppb * ppb;

            // Triply indirect
            uint32_t tib = inode->triply_indirect_block_pointer;
            uint32_t l1 = ensure_indirect(file, device, &tib, block_size);
            inode->triply_indirect_block_pointer = tib;
            if (l1 != 0) {
                uint32_t l2 = 0;
                vfs_seek(
                    file,
                    (uint64_t)l1 * block_size
                        + (block_id / (ppb * ppb)) * sizeof(uint32_t)
                );
                vfs_read(file, sizeof(uint32_t), &l2);

                if (l2 == 0) {
                    l2 = ext2_alloc_block(device, block_size);
                    if (l2 != 0) {
                        uint8_t* zeros = kmalloc_flags(
                            block_size,
                            KMALLOC_ZERO
                        );
                        vfs_seek(file, (uint64_t)l2 * block_size);
                        vfs_write(file, block_size, zeros);
                        kfree(zeros);
                        vfs_seek(
                            file,
                            (uint64_t)l1 * block_size
                                + (block_id / (ppb * ppb)) * sizeof(uint32_t)
                        );
                        vfs_write(file, sizeof(uint32_t), &l2);
                    }
                }
                if (l2 != 0) {
                    uint32_t l3 = 0;
                    vfs_seek(
                        file,
                        (uint64_t)l2 * block_size
                            + ((block_id / ppb) % ppb) * sizeof(uint32_t)
                    );
                    vfs_read(file, sizeof(uint32_t), &l3);

                    if (l3 == 0) {
                        l3 = ext2_alloc_block(device, block_size);
                        if (l3 != 0) {
                            uint8_t* zeros = kmalloc_flags(
                                block_size,
                                KMALLOC_ZERO
                            );
                            vfs_seek(file, (uint64_t)l3 * block_size);
                            vfs_write(file, block_size, zeros);
                            kfree(zeros);
                            vfs_seek(
                                file,
                                (uint64_t)l2 * block_size
                                    + ((block_id/ppb) % ppb) * sizeof(uint32_t)
                            );
                            vfs_write(file, sizeof(uint32_t), &l3);
                        }
                    }
                    if (l3 != 0) {
                        vfs_seek(
                            file,
                            (uint64_t)l3 * block_size
                            + (block_id % ppb) * sizeof(uint32_t)
                        );
                        vfs_write(file, sizeof(uint32_t), &new_block_num);
                        ok = true;
                    }
                }
            }
        }
    }

    vfs_close(file);
    return ok;
}
