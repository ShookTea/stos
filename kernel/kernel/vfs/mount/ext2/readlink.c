#include "./ext2.h"
#include <errno.h>
#include "kernel/vfs/vfs.h"
#include "kernel/debug.h"

#define _debug_puts(...) debug_puts_c(DBC_VFS_EXT2, __VA_ARGS__)
#define _debug_printf(...) debug_printf_c(DBC_VFS_EXT2, __VA_ARGS__)

int ext2_readlink(const vfs_node_t* node, char* buf, size_t len)
{
    ext2_inode_metadata_t* meta = node->metadata;
    if (meta == NULL || meta->cached_inode == NULL) {
        return -EINVAL;
    }
    ext2_inode_t* ino = meta->cached_inode;
    if (ext2_type_to_vfs(ino->type_and_permissions) != VFS_TYPE_SYMLINK) {
        return -EINVAL;
    }

    _debug_printf(
        "Running readlink on %u '%s' - len=%u\n",
        node->inode,
        node->filename,
        len
    );

    vfs_file_t* device_file = vfs_open(meta->device_file, VFS_MODE_READONLY);
    if (device_file == NULL) {
        _debug_puts("device file = NULL");
        return -EIO;
    }

    // Update file's last access time
    meta->cached_inode->last_access_time = (uint32_t)time(NULL);
    ext2_write_inode(
        meta->device_file,
        node->inode,
        meta->block_size,
        meta->inode_size,
        meta->inodes_per_group,
        meta->cached_inode
    );

    // Calculate end block indice
    if (len > node->length) {
        len = node->length;
    }
    size_t end_block_id = len / meta->block_size;

    // Reading blocks
    size_t bytes_read = 0;
    for (size_t i = 0; i <= end_block_id; i++) {
        size_t block_file_start = i * meta->block_size;
        size_t bytes_this_block = (i == end_block_id)
            ? len
            : block_file_start + meta->block_size;
        if (bytes_this_block == 0) {
            break;
        }

        size_t block_addr = ext2_block_id_to_addr(
            meta->device_file,
            ino,
            i,
            meta->block_size
        );
        if (block_addr == 0) {
            break;
        }
        vfs_seek(device_file, block_addr);
        vfs_read(device_file, bytes_this_block, buf + bytes_read);
        bytes_read += bytes_this_block;
    }

    vfs_close(device_file);
    return bytes_read;
}
