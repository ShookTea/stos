#include <errno.h>
#include "kernel/task/scheduler.h"
#include "kernel/task/syscall.h"
#include "kernel/task/task.h"
#include "kernel/vfs/vfs.h"

/**
 * Handle case where user didn't specify newfd
 */
static int sys_dup_no_newfd(
    task_t* current,
    task_file_descriptor_t* fd_obj
) {
    // Duplicate file descriptor
    task_file_descriptor_t* fd_new_obj = task_add_fd(current, fd_obj->file);
    if (fd_new_obj == NULL) {
        return -EIO;
    }

    vfs_bump_refcount(fd_new_obj->file);
    return fd_new_obj->identifier;
}

/**
 * Handle case where user specified newfd
 */
static int sys_dup_with_newfd(
    task_t* current,
    task_file_descriptor_t* fd_obj,
    int newfd
) {
    // First, try to search for existing instance with `newfd`
    task_file_descriptor_t* existing_newfd = NULL;
    for (size_t i = 0; i < current->fd_count; i++) {
        task_file_descriptor_t* tmp = current->fd[i];
        if (tmp->identifier == (uint32_t)newfd) {
            existing_newfd = tmp;
            break;
        }
    }

    if (existing_newfd == NULL) {
        // Given newfd identifier was never opened - just clone a descriptor
        // and update it's identifier
        task_file_descriptor_t* fd_new_obj = task_add_fd(current, fd_obj->file);
        if (fd_new_obj == NULL) {
            return -EIO;
        }
        fd_new_obj->identifier = (uint32_t)newfd;
        vfs_bump_refcount(fd_obj->file);
        return newfd;
    }

    if (existing_newfd->file != NULL) {
        // The FD exists and is used right now. We need to close it first.
        vfs_close(existing_newfd->file);
    }

    // Now we can reuse reuse it
    existing_newfd->file = fd_obj->file;
    vfs_bump_refcount(fd_obj->file);
    return newfd;
}

int sys_dup(int oldfd, int newfd, int flags)
{
    if (flags != 0 || oldfd < 0) {
        return -EINVAL;
    }

    task_t* current = scheduler_get_current_task();
    if (current == NULL) {
        return -ENOTSUP;
    }

    task_file_descriptor_t* fd_obj = syscall_get_descriptor_by_fd(oldfd);
    if (fd_obj == NULL) {
        return -EBADF;
    }

    if (newfd == -1) {
        return sys_dup_no_newfd(current, fd_obj);
    }

    return sys_dup_with_newfd(current, fd_obj, newfd);
}
