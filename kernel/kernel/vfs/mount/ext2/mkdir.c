#include <stdbool.h>
#include <string.h>
#include "kernel/vfs/vfs.h"
#include "kernel/memory/kmalloc.h"
#include "./ext2.h"
#include "kernel/debug.h"

#define _debug_puts(...) debug_puts_c("VFS/mount/ext2", __VA_ARGS__)
#define _debug_printf(...) debug_printf_c("VFS/mount/ext2", __VA_ARGS__)

/**
 * Returns `true` if there's already a node with given `name` in `node`.
 */
static bool check_if_exists(vfs_node_t* node, const char* name)
{
    // Try to verify if given name already exists
    char* name2 = kmalloc(sizeof(char) * strlen(name) + 1);
    strcpy(name2, name);
    vfs_node_t* existing = ext2_finddir(node, name2);
    kfree(name2);

    if (existing != NULL) {
        existing->on_release(existing);
        return true;
    }

    return false;
}

bool ext2_mkdir(vfs_node_t* node, const char* name)
{
    _debug_printf(
        "Run mkdir with name '%s' on inode %u ('%s')\n",
        name,
        node->inode,
        node->filename
    );

    if ((node->type & VFS_TYPE_DIRECTORY) == 0) {
        _debug_puts("Parent node is not a directory");
        return false;
    }

    if (check_if_exists(node, name)) {
        _debug_puts("Parent node already contains a node with given name");
        return false;
    }

    _debug_puts("creating dir");

    return false;
}
