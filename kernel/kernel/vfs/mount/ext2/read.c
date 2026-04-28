#include <stddef.h>
#include "kernel/vfs/vfs.h"
#include "./ext2.h"

size_t ext2_read(vfs_file_t* file, size_t offset, size_t size, void* ptr)
{
    if (file == NULL || ptr == NULL || file->metadata == NULL)
    {
        return 0;
    }
    if (!file->readable) {
        return 0;
    }

    // Adjust start and end based of file size
    size_t file_size = file->dentry->inode->length;
    if (offset >= file_size) {
        return 0;
    }
    if ((offset + size) > file_size) {
        size = file_size - offset;
    }
    if (size == 0) {
        return 0;
    }

    // Load metadata
    ext2_file_metadata_t* meta = file->metadata;
    ext2_inode_t* inode = meta->cached_inode;
    dentry_t* device_file_dentry = meta->device_file;

    // Calculate start and end block indices
    size_t start_block_id = offset / meta->block_size;
    size_t end_block_id = (offset + size) / meta->block_size;
    // Calculate offset (in bytes) inside the block
    size_t start_block_offset = offset % meta->block_size;
    size_t end_block_offset = (offset + size) % meta->block_size;

    // Open device file
    vfs_file_t* device_file = vfs_open(device_file_dentry, VFS_MODE_READONLY);

    for (size_t i = start_block_id; i <= end_block_id; i++) {

    }
}
