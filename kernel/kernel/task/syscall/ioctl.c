#include "../syscall.h"
#include "kernel/task/scheduler.h"
#include "kernel/task/task.h"
#include "kernel/vfs/vfs.h"

static task_file_descriptor_t* get_descriptor(int fd)
{
    task_t* task = scheduler_get_current_task();
    if (task == NULL) {
        return NULL;
    }

    // Validate wrong FD values
    if (fd < 0) {
        return NULL;
    }
    if (fd < 3) {
        // Reserved for stdin/out/err; currenty unsupported
        // TODO: add support
        return NULL;
    }

    for (size_t i = 0; i < task->fd_count; i++) {
        if (task->fd[i]->file != NULL
            && task->fd[i]->identifier == (uint32_t)fd) {
            return task->fd[i];
        }
    }
    return NULL;
}

int sys_ioctl(int fd_id, int op, void* arg)
{
    // Get file descriptor
    task_file_descriptor_t* fd = get_descriptor(fd_id);
    if (fd == NULL) {
        return SYSCALL_ERROR;
    }

    vfs_file_t* file = fd->file;

    if (file->dentry->inode->ioctl_node == NULL) {
        return SYSCALL_ERROR; // TODO: mark correct error status ("not a TTY")
    }

    return fd->file->dentry->inode->ioctl_node(file, op, arg);
}
