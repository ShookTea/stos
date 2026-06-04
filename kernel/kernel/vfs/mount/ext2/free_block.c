#include <stdint.h>
#include <fcntl.h>
#include "kernel/memory/kmalloc.h"
#include "kernel/vfs/vfs.h"
#include "./ext2.h"

void ext2_free_block(dentry_t* device, uint32_t block_num, uint32_t block_size)
{
    vfs_file_t* file = vfs_open(device, O_RDWR, NULL);
    if (file == NULL) return;

    ext2_superblock_t* sb = kmalloc_flags(
        sizeof(ext2_superblock_t),
        KMALLOC_ZERO
    );
    vfs_seek(file, 1024);
    vfs_read(file, sizeof(ext2_superblock_t), sb);

    uint32_t gdt_start = (block_size == 1024 ? 2u : 1u) * block_size;
    uint32_t blocks_per_group = sb->blocks_per_group;
    uint32_t group = block_num / blocks_per_group;
    uint32_t bit   = block_num % blocks_per_group;

    ext2_gdt_t* gdt = kmalloc_flags(sizeof(ext2_gdt_t), KMALLOC_ZERO);
    vfs_seek(file, gdt_start + (uint64_t)group * sizeof(ext2_gdt_t));
    vfs_read(file, sizeof(ext2_gdt_t), gdt);

    uint8_t* bitmap = kmalloc_flags(block_size, KMALLOC_ZERO);
    vfs_seek(file, (uint64_t)gdt->block_usage_bitmap_addr * block_size);
    vfs_read(file, block_size, bitmap);

    bitmap[bit / 8] &= ~(1u << (bit % 8));

    vfs_seek(file, (uint64_t)gdt->block_usage_bitmap_addr * block_size);
    vfs_write(file, block_size, bitmap);

    gdt->unalloc_blocks_count++;
    vfs_seek(file, gdt_start + (uint64_t)group * sizeof(ext2_gdt_t));
    vfs_write(file, sizeof(ext2_gdt_t), gdt);

    sb->total_unalloc_blocks++;
    vfs_seek(file, 1024);
    vfs_write(file, sizeof(ext2_superblock_t), sb);

    kfree(bitmap);
    kfree(gdt);
    kfree(sb);
    vfs_close(file);
}
