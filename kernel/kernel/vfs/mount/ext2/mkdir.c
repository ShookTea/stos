#include <stdbool.h>
#include "kernel/vfs/vfs.h"
#include "./ext2.h"
#include "kernel/debug.h"

#define _debug_puts(...) debug_puts_c(DBC_VFS_EXT2, __VA_ARGS__)
#define _debug_printf(...) debug_printf_c(DBC_VFS_EXT2, __VA_ARGS__)

bool ext2_mkdir(vfs_node_t* node, const char* name)
{
    ext2_inode_metadata_t* meta = node->metadata;
    _debug_printf(
        "Run mkdir with name '%s' on inode %u ('%s')\n",
        name,
        node->inode,
        node->filename
    );

    ext2_inode_t new_inode = {0};
    uint32_t ino = ext2_create_inode(
        node,
        name,
        EXT2_TYPE_DIRECTORY,
        &new_inode
    );

    if (ino == 0) {
        _debug_puts("Failed to create inode for directory");
        return false;
    }

    // Add "." and ".." to the new directory's content
    if (!ext2_add_inode_to_dir(meta, ino, ino, ".")) {
        _debug_puts("Failed to create '.' entry");
        return false;
        // TODO: cleanup - dealloc inode
    }
    if (!ext2_add_inode_to_dir(meta, ino, node->inode, "..")) {
        _debug_puts("Failed to create '..' entry");
        return false;
        // TODO: cleanup - remove "." and dealloc inode
    }

    return true;
}
