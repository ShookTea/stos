#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include "kernel/vfs/vfs.h"
#include "kernel/memory/kmalloc.h"
#include "./ext2.h"

/*
 * Free all data blocks pointed to by a singly-indirect block, then free
 * the indirect block itself.
 */
static void free_indirect1(
    vfs_file_t* dev,
    dentry_t* device,
    uint32_t block,
    uint32_t block_size,
    uint32_t ppb
) {
    uint32_t* table = kmalloc_flags(block_size, KMALLOC_ZERO);
    vfs_seek(dev, (uint64_t)block * block_size);
    vfs_read(dev, block_size, table);
    for (uint32_t i = 0; i < ppb; i++) {
        if (table[i] != 0) {
            ext2_free_block(device, table[i], block_size);
        }
    }
    kfree(table);
    ext2_free_block(device, block, block_size);
}

static void free_indirect2(
    vfs_file_t* dev,
    dentry_t* device,
    uint32_t block,
    uint32_t block_size,
    uint32_t ppb
) {
    uint32_t* l1 = kmalloc_flags(block_size, KMALLOC_ZERO);
    vfs_seek(dev, (uint64_t)block * block_size);
    vfs_read(dev, block_size, l1);
    for (uint32_t i = 0; i < ppb; i++) {
        if (l1[i] != 0) {
            free_indirect1(dev, device, l1[i], block_size, ppb);
        }
    }
    kfree(l1);
    ext2_free_block(device, block, block_size);
}

static void free_indirect3(
    vfs_file_t* dev,
    dentry_t* device,
    uint32_t block,
    uint32_t block_size,
    uint32_t ppb
) {
    uint32_t* l1 = kmalloc_flags(block_size, KMALLOC_ZERO);
    vfs_seek(dev, (uint64_t)block * block_size);
    vfs_read(dev, block_size, l1);
    for (uint32_t i = 0; i < ppb; i++) {
        if (l1[i] != 0) {
            free_indirect2(dev, device, l1[i], block_size, ppb);
        }
    }
    kfree(l1);
    ext2_free_block(device, block, block_size);
}

int ext2_truncate(vfs_file_t* file)
{
    ext2_file_metadata_t* meta = file->metadata;
    ext2_inode_t* inode = meta->cached_inode;
    dentry_t* device = meta->device_file;
    uint32_t block_size = meta->block_size;
    uint32_t ppb = block_size / sizeof(uint32_t);

    if (file->dentry->inode->length == 0) {
        return 0;
    }

    vfs_file_t* dev = vfs_open(device, O_RDONLY, NULL);
    if (dev == NULL) return EIO;

    for (int i = 0; i < 12; i++) {
        if (inode->direct_block_pointers[i] != 0) {
            ext2_free_block(
                device,
                inode->direct_block_pointers[i],
                block_size
            );
            inode->direct_block_pointers[i] = 0;
        }
    }

    if (inode->singly_indirect_block_pointer != 0) {
        free_indirect1(
            dev,
            device,
            inode->singly_indirect_block_pointer,
            block_size,
            ppb
        );
        inode->singly_indirect_block_pointer = 0;
    }
    if (inode->doubly_indirect_block_pointer != 0) {
        free_indirect2(
            dev,
            device,
            inode->doubly_indirect_block_pointer,
            block_size,
            ppb
        );
        inode->doubly_indirect_block_pointer = 0;
    }
    if (inode->triply_indirect_block_pointer != 0) {
        free_indirect3(
            dev,
            device,
            inode->triply_indirect_block_pointer,
            block_size,
            ppb
        );
        inode->triply_indirect_block_pointer = 0;
    }

    vfs_close(dev);

    inode->size_lo = 0;
    inode->disk_sectors_count = 0;
    inode->last_modification_time = (uint32_t)time(NULL);

    file->dentry->inode->length = 0;

    // Keep the node-level inode cache consistent so a subsequent open
    // without re-reading from disk doesn't see stale block pointers.
    ext2_inode_metadata_t* node_meta = file->dentry->inode->metadata;
    if (node_meta->cached_inode != NULL) {
        memcpy(node_meta->cached_inode, inode, sizeof(ext2_inode_t));
    }

    ext2_write_inode(
        device,
        file->dentry->inode->inode,
        block_size,
        meta->inode_size,
        meta->inodes_per_group,
        inode
    );

    return 0;
}
