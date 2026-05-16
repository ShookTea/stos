#include <stddef.h>
#include <errno.h>
#include "../syscall.h"
#include "kernel/task/scheduler.h"
#include "kernel/task/task.h"
#include "kernel/vfs/vfs.h"
#include <kernel/memory/vmm.h>
#include <kernel/terminal.h>

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

int sys_lseek(int fd_id, int offset, int whence)
{
    // Get file descriptor
    task_file_descriptor_t* fd = get_descriptor(fd_id);
    if (fd == NULL) {
        // TODO: set error
        return -EBADF;
    }

    int new_offset = 0;
    switch (whence) {
        case SYS_LSEEK_OPT_SEEK_SET:
            new_offset = offset;
            break;
        case SYS_LSEEK_OPT_SEEK_CUR:
            new_offset = fd->file->offset + offset;
            break;
        case SYS_LSEEK_OPT_SEEK_END:
            new_offset = fd->file->dentry->inode->length + offset;
            break;
        default:
            return -EINVAL;
    }

    vfs_seek(fd->file, new_offset);

    return fd->file->offset;
}
