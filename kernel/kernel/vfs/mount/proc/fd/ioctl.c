#include <errno.h>
#include "../proc.h"

int proc_fd_ioctl(vfs_file_t* file, uint32_t op, void* arg)
{
    proc_file_meta_fd_t* meta = file->metadata;
    if (meta == NULL) {
        return -ENOTTY;
    }

    task_file_descriptor_t* fd;
    if (!proc_find_fd(meta->pid, meta->fd_id, &fd)) {
        return -ENOTTY;
    }

    vfs_file_t* real = meta->opened_file;
    if (real == NULL || real->dentry == NULL || real->dentry->inode == NULL) {
        return -ENOTTY;
    }

    vfs_node_t* real_node = real->dentry->inode;
    if (real_node->ioctl_node == NULL) {
        return -ENOTTY;
    }

    return real_node->ioctl_node(real, op, arg);
}
