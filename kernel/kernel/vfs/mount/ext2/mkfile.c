#include <stdbool.h>
#include <errno.h>
#include "kernel/vfs/vfs.h"
#include "./ext2.h"
#include "kernel/debug.h"

#define _debug_puts(...) debug_puts_c(DBC_VFS_EXT2, __VA_ARGS__)
#define _debug_printf(...) debug_printf_c(DBC_VFS_EXT2, __VA_ARGS__)

int ext2_mkfile(vfs_node_t* node, const char* name)
{
    _debug_printf(
        "Run mkfile with name '%s' on inode %u ('%s')\n",
        name,
        node->inode,
        node->filename
    );

    ext2_inode_t new_inode = {0};
    uint32_t ino = ext2_create_inode(
        node,
        name,
        EXT2_TYPE_FILE,
        &new_inode
    );

    if (ino == 0) {
        _debug_puts("Failed to create inode for file");
        return ENOTSUP; // TODO: implement proper errors
    }

    return 0;
}
