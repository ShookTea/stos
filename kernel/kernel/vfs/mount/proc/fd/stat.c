#include <errno.h>
#include "../proc.h"
#include "kernel/vfs/vfs.h"

int proc_fd_stat(const vfs_node_t* node, struct stat* stat)
{
    proc_inode_meta_fd_t* meta = node->metadata;
    if (meta == NULL) {
        return ENOTSUP;
    }

    task_file_descriptor_t* fd;
    if (!proc_find_fd(meta->pid, meta->fd_id, &fd)) {
        return ENOTSUP;
    }

    if (fd->file == NULL || fd->file->dentry == NULL) {
        return ENOTSUP;
    }
    dentry_t* dentry = fd->file->dentry;
    if (dentry->inode == NULL || dentry->inode->stat_node == NULL) {
        return ENOTSUP;
    }

    return dentry->inode->stat_node(dentry->inode, stat);
}
