#include <errno.h>
#include "../syscall.h"
#include "kernel/task/task.h"
#include "kernel/vfs/vfs.h"


int sys_ioctl(int fd_id, int op, void* arg)
{
    // Get file descriptor
    task_file_descriptor_t* fd = syscall_get_descriptor_by_fd(fd_id);
    if (fd == NULL) {
        return -EBADF;
    }

    vfs_file_t* file = fd->file;

    if (file->dentry->inode->ioctl_node == NULL) {
        return -ENOTTY;
    }

    return fd->file->dentry->inode->ioctl_node(file, op, arg);
}
