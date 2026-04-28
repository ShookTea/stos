#include <stddef.h>
#include "kernel/vfs/vfs.h"
#include "./ext2.h"
#include "kernel/debug.h"

#define _debug_puts(...) debug_puts_c("VFS/mount/ext2", __VA_ARGS__)
#define _debug_printf(...) debug_printf_c("VFS/mount/ext2", __VA_ARGS__)

size_t ext2_read(vfs_file_t* file, size_t offset, size_t size, void* ptr)
{
    _debug_printf(
        "Reading inode %u '%s' - off=%u size=%u \n",
        file->dentry->inode->inode,
        file->dentry->name,
        offset,
        size
    );

    if (file == NULL || ptr == NULL || file->metadata == NULL) {
        _debug_puts("nullpointer at ext2 read");
        return 0;
    }
    if (!file->readable) {
        _debug_puts("file not readable");
        return 0;
    }

    // Adjust start and end based of file size
    size_t file_size = file->dentry->inode->length;
    if (offset >= file_size) {
        _debug_printf("offset greater than filesize %u\n", file_size);
        return 0;
    }
    if ((offset + size) > file_size) {
        size = file_size - offset;
    }
    if (size == 0) {
        _debug_puts("Readable size = 0");
        return 0;
    }

    // Load metadata
    ext2_file_metadata_t* meta = file->metadata;
    ext2_inode_t* inode = meta->cached_inode;
    _debug_printf("creation time = %u\n", inode->creation_time);
    dentry_t* device_file_dentry = meta->device_file;

    // Calculate start and end block indices
    size_t start_block_id = offset / meta->block_size;
    size_t end_block_id = (offset + size) / meta->block_size;
    // Open device file
    vfs_file_t* device_file = vfs_open(device_file_dentry, VFS_MODE_READONLY);
    if (device_file == NULL) {
        _debug_puts("device file = NULL");
        return 0;
    }

    uint8_t* out = (uint8_t*)ptr;
    size_t bytes_read = 0;

    for (size_t i = start_block_id; i <= end_block_id; i++) {
        size_t block_file_start = i * meta->block_size;

        // Clamp read range to [offset, offset+size) intersected with this block
        size_t read_start = (i == start_block_id) ? offset : block_file_start;
        size_t read_end = (i == end_block_id)
            ? offset + size
            : block_file_start + meta->block_size;
        size_t bytes_this_block = read_end - read_start;

        if (bytes_this_block == 0) {
            break;
        }

        size_t block_addr = ext2_block_id_to_addr(
            device_file_dentry,
            inode,
            i,
            meta->block_size
        );
        if (block_addr == 0) {
            break;
        }

        size_t src_off = read_start - block_file_start;
        vfs_seek(device_file, block_addr + src_off);
        vfs_read(device_file, bytes_this_block, out + bytes_read);
        bytes_read += bytes_this_block;
    }

    vfs_close(device_file);
    return bytes_read;
}
