#include <stdint.h>
#include <string.h>
#include "./ext2.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/vfs/vfs.h"
#include "kernel/debug.h"

#define _debug_puts(...) debug_puts_c("VFS/mount/ext2", __VA_ARGS__)
#define _debug_printf(...) debug_printf_c("VFS/mount/ext2", __VA_ARGS__)

void ext2_open(vfs_node_t* node, vfs_file_t* file, uint8_t mode)
{
    _debug_printf(
        "Opening inode %u '%s' in mode 0x%02X\n",
        node->inode,
        file->dentry->name,
        mode
    );
    ext2_inode_metadata_t* node_meta = node->metadata;

    if (!ext2_ensure_dir_cache(node)) {
        return;
    }

    ext2_file_metadata_t* meta = kmalloc_flags(
        sizeof(ext2_file_metadata_t),
        KMALLOC_ZERO
    );
    meta->cached_inode = kmalloc_flags(
        sizeof(ext2_inode_t),
        KMALLOC_ZERO
    );
    meta->device_file = kmalloc_flags(
        sizeof(dentry_t),
        KMALLOC_ZERO
    );

    memcpy(meta->cached_inode, node_meta->cached_inode, sizeof(ext2_inode_t));
    memcpy(meta->device_file, node_meta->device_file, sizeof(dentry_t));
    meta->block_size = node_meta->block_size;
    meta->inode_size = node_meta->inode_size;
    meta->inodes_per_group = node_meta->inodes_per_group;
    file->metadata = meta;
}
