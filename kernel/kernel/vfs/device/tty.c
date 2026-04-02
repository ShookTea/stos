#include "kernel/vfs/device.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/vfs/vfs.h"

static vfs_node_t* node = NULL;

vfs_node_t* device_tty_mount()
{
    if (node != NULL) {
        return node;
    }

    node = kmalloc(sizeof(vfs_node_t));
    vfs_populate_node(node, "tty", VFS_TYPE_CHARACTER_DEVICE);
    return node;
}
