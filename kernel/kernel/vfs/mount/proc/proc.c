#include "proc.h"
#include "../mount.h"
#include "kernel/vfs/vfs.h"
#include "kernel/memory/kmalloc.h"

void proc_on_release(vfs_node_t* node)
{
    if (node == NULL) {
        return;
    }

    if (node->metadata != NULL) {
        kfree(node->metadata);
    }
    kfree(node);
}

vfs_mount_result_t vfs_mount_proc(
    dentry_t* target
) {
    target->inode->readdir_node = proc_root_readdir;
    target->inode->finddir_node = proc_root_finddir;
    return MOUNT_SUCCESS;
}
