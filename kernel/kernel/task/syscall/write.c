#include <stddef.h>
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

int sys_write(int fd_id, const void* buf, size_t count)
{
    if (fd_id == 1 || fd_id == 2) {
        // stdout (fd=1) and stderr (fd=2) are currently handled by
        // terminal_write_char. TODO: Ideally we should use the VFS system.
        const char* str = (const char*)buf;
        for (size_t i = 0; i < count; i++) {
            terminal_write_char(str[i]);
        }
        return (int)count;
    }

    task_file_descriptor_t* fd = get_descriptor(fd_id);
    if (fd == NULL) {
        return -1;
    }

    size_t result = vfs_write(fd->file, count, buf);
    return result;
}
