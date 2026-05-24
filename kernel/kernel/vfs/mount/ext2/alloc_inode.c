#include <stdint.h>
#include <fcntl.h>
#include "ext2.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/vfs/vfs.h"

uint32_t ext2_alloc_inode(
    dentry_t* device,
    uint32_t block_size
) {
    vfs_file_t* file = vfs_open(device, O_RDONLY);
    if (file == NULL) {
        return 0;
    }

    ext2_superblock_t* sb = kmalloc_flags(
        sizeof(ext2_superblock_t),
        KMALLOC_ZERO
    );
    vfs_seek(file, 1024);
    vfs_read(file, sizeof(ext2_superblock_t), sb);

    if (sb->total_unalloc_inodes == 0) {
        kfree(sb);
        vfs_close(file);
        return 0;
    }

    uint32_t gdt_start = (block_size == 1024 ? 2u : 1u) * block_size;
    uint32_t inodes_per_group = sb->inodes_per_group;
    uint32_t groups_count =
        (sb->total_inodes + inodes_per_group - 1) / inodes_per_group;

    uint8_t* bitmap = kmalloc_flags(block_size, KMALLOC_ZERO);
    ext2_gdt_t* gdt = kmalloc_flags(sizeof(ext2_gdt_t), KMALLOC_ZERO);

    uint32_t result = 0;
    for (uint32_t g = 0; g < groups_count && result == 0; g++) {
        vfs_seek(file, gdt_start + (uint64_t)g * sizeof(ext2_gdt_t));
        vfs_read(file, sizeof(ext2_gdt_t), gdt);

        if (gdt->unalloc_inodes_count == 0) {
            continue;
        }

        vfs_seek(file, (uint64_t)gdt->inode_usage_bitmap_addr * block_size);
        vfs_read(file, block_size, bitmap);

        uint32_t inodes_in_group = (g == groups_count - 1)
            ? sb->total_inodes - g * inodes_per_group
            : inodes_per_group;

        for (uint32_t bit = 0; bit < inodes_in_group; bit++) {
            if ((bitmap[bit / 8] & (1u << (bit % 8))) != 0) {
                continue;
            }
            // Adding "+1" because ext2 inode numbers start at 1
            uint32_t ino = g * inodes_per_group + bit + 1;

            if (ino < sb->first_unreserved_inode) {
                continue; // Ignore reserved inodes
            }

            bitmap[bit / 8] |= (1u << (bit % 8));
            vfs_seek(file, (uint64_t)gdt->inode_usage_bitmap_addr * block_size);
            vfs_write(file, block_size, bitmap);

            gdt->unalloc_inodes_count--;
            vfs_seek(file, gdt_start + (uint64_t)g * sizeof(ext2_gdt_t));
            vfs_write(file, sizeof(ext2_gdt_t), gdt);

            sb->total_unalloc_inodes--;
            vfs_seek(file, 1024);
            vfs_write(file, sizeof(ext2_superblock_t), sb);

            result = ino;
            break;
        }
    }

    kfree(bitmap);
    kfree(gdt);
    kfree(sb);
    vfs_close(file);
    return result;
}
