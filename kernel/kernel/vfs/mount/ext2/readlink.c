#include <errno.h>
#include <string.h>
#include "./ext2.h"
#include "kernel/vfs/vfs.h"
#include "kernel/debug.h"
#include "kernel/memory/kmalloc.h"

#define _debug_puts(...) debug_puts_c(DBC_VFS_EXT2, __VA_ARGS__)
#define _debug_printf(...) debug_printf_c(DBC_VFS_EXT2, __VA_ARGS__)

int ext2_readlink(const vfs_node_t* node, char* buf, size_t len)
{
    ext2_inode_metadata_t* meta = node->metadata;
    if (meta == NULL) {
        return -EIO;
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

    ext2_inode_t* ino = ext2_read_inode(device_file, meta, node->inode);
    if (ino == NULL) {
        vfs_close(device_file);
        return -ENOENT;
    }

    if (ext2_type_to_vfs(ino->type_and_permissions) != VFS_TYPE_SYMLINK) {
        kfree(ino);
        vfs_close(device_file);
        return -EINVAL;
    }

    // Update file's last access time
    ino->last_access_time = (uint32_t)time(NULL);
    ext2_write_inode(
        meta->device_file,
        node->inode,
        meta->block_size,
        meta->inode_size,
        meta->inodes_per_group,
        ino
    );

    // Fast symlinks: target is stored inline in the inode's block pointer area
    if (ino->disk_sectors_count == 0) {
        size_t copy_len = len < node->length ? len : node->length;
        memcpy(buf, ino->direct_block_pointers, copy_len);
        vfs_close(device_file);
        kfree(ino);
        return (int)copy_len;
    }

    // Calculate end block index
    if (len > node->length) {
        len = node->length;
    }
    size_t end_block_id = len / meta->block_size;

    // Reading blocks
    size_t bytes_read = 0;
    for (size_t i = 0; i <= end_block_id; i++) {
        size_t bytes_this_block = (i == end_block_id)
            ? len - bytes_read
            : meta->block_size;
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
    kfree(ino);
    return bytes_read;
}
