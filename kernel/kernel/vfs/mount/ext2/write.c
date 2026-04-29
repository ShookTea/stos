#include <stddef.h>
#include <stdint.h>
#include "kernel/vfs/vfs.h"
#include "./ext2.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/debug.h"

#define _debug_puts(...) debug_puts_c("VFS/mount/ext2", __VA_ARGS__)
#define _debug_printf(...) debug_printf_c("VFS/mount/ext2", __VA_ARGS__)

size_t ext2_write(vfs_file_t* file, size_t offset, size_t size, const void* ptr)
{
    _debug_printf(
        "Writing to inode %u '%s' - off=%u size=%u \n",
        file->dentry->inode->inode,
        file->dentry->name,
        offset,
        size
    );

    if (file == NULL || ptr == NULL || file->metadata == NULL) {
        _debug_puts("nullpointer at ext2 write");
        return 0;
    }
    if (!file->writeable) {
        _debug_puts("file not writeable");
        return 0;
    }

    size_t file_size = file->dentry->inode->length;
    if (offset > file_size) {
        _debug_printf("offset beyond filesize %u\n", file_size);
        return 0;
    }
    if (size == 0) {
        _debug_puts("Write size = 0");
        return 0;
    }

    // Load metadata
    ext2_file_metadata_t* meta = file->metadata;
    ext2_inode_t* inode = meta->cached_inode;
    dentry_t* device_file_dentry = meta->device_file;

    // Calculate start and end block indices
    size_t start_block_id = offset / meta->block_size;
    size_t end_block_id = (offset + size) / meta->block_size;

    vfs_file_t* device_file = vfs_open(device_file_dentry, VFS_MODE_WRITEONLY);
    if (device_file == NULL) {
        _debug_puts("device file = NULL");
        return 0;
    }

    const uint8_t* in = (const uint8_t*)ptr;
    size_t bytes_written = 0;
    bool inode_dirty = false;
    uint8_t* zeros = NULL;

    for (size_t i = start_block_id; i <= end_block_id; i++) {
        size_t block_file_start = i * meta->block_size;

        size_t write_start = (i == start_block_id) ? offset : block_file_start;
        size_t write_end = (i == end_block_id)
            ? offset + size
            : block_file_start + meta->block_size;
        size_t bytes_this_block = write_end - write_start;

        if (bytes_this_block == 0) {
            break;
        }

        size_t block_addr = ext2_block_id_to_addr(
            device_file_dentry, inode, i, meta->block_size);

        if (block_addr == 0) {
            uint32_t new_block = ext2_alloc_block(
                device_file_dentry,
                meta->block_size
            );
            if (new_block == 0) {
                _debug_puts("ext2_alloc_block failed - disk full?");
                break;
            }
            bool set_block_id_res = ext2_set_block_id(
                device_file_dentry,
                inode,
                i,
                meta->block_size,
                new_block
            );
            if (!set_block_id_res) {
                _debug_puts("ext2_set_block_id failed");
                break;
            }

            if (zeros == NULL) {
                // Zero the new block so partial writes leave the rest clean
                zeros = kmalloc_flags(meta->block_size, KMALLOC_ZERO);
            }
            vfs_seek(device_file, (uint64_t)new_block * meta->block_size);
            vfs_write(device_file, meta->block_size, zeros);

            block_addr = (size_t)new_block * meta->block_size;
            inode_dirty = true;
        }

        size_t dst_off = write_start - block_file_start;
        vfs_seek(device_file, block_addr + dst_off);
        vfs_write(device_file, bytes_this_block, in + bytes_written);
        bytes_written += bytes_this_block;
    }

    if (zeros != NULL) kfree(zeros);
    vfs_close(device_file);

    size_t new_end = offset + bytes_written;
    if (new_end > file_size) {
        inode->size_lo = (uint32_t)new_end;
        file->dentry->inode->length = new_end;
        inode_dirty = true;
    }

    if (inode_dirty) {
        ext2_write_inode(
            device_file_dentry,
            file->dentry->inode->inode,
            meta->block_size,
            meta->inode_size,
            meta->inodes_per_group,
            inode
        );
    }

    return bytes_written;
}
