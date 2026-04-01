#include "../syscall.h"
#include "kernel/task/scheduler.h"
#include "kernel/task/task.h"
#include "kernel/vfs/vfs.h"
#include <stdint.h>

static task_file_descriptor_t* get_descriptor_by_fd(int fd)
{
    task_t* current = scheduler_get_current_task();
    if (current == NULL) {
        return NULL;
    }

    for (size_t i = 0; i < current->fd_count; i++) {
        if (current->fd[i]->file != NULL
            && current->fd[i]->identifier == (uint32_t)fd) {
            return current->fd[i];
        }
    }

    return NULL;
}

int sys_close(int fd)
{
    task_file_descriptor_t* desc = get_descriptor_by_fd(fd);
    if (desc == NULL) {
        return SYSCALL_ERROR;
    }

    vfs_close(desc->file);
    desc->file = NULL;

    return SYSCALL_SUCCESS;
}
