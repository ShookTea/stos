#include "kernel/task/syscall.h"
#include <errno.h>
#include "kernel/memory/kmalloc.h"
#include "kernel/task/scheduler.h"
#include "kernel/task/task.h"
#include "kernel/vfs/vfs.h"
#include <stdint.h>

uint32_t sys_open(const char* path, uint32_t flags)
{
    task_t* current = scheduler_get_current_task();
    if (current == NULL) {
        return -ENOTSUP;
    }

    dentry_t* node = vfs_resolve_relative(
        current->root_node,
        current->working_directory,
        path
    );
    if (node == NULL) {
        return -ENOENT;
    }

    vfs_file_t* handler = vfs_open(node, flags, NULL);
    if (handler == NULL) {
        return -EIO;
    }

    // First try to find first existing identifier that is already allocated but
    // was freed before
    for (size_t i = 0; i < current->fd_count; i++) {
        task_file_descriptor_t* desc = current->fd[i];
        if (desc->file == NULL) {
            desc->file = handler;
            return desc->identifier;
        }
    }

    // Allocate new entry for file descriptor
    task_file_descriptor_t* desc = kmalloc(sizeof(task_file_descriptor_t));
    desc->file = handler;
    desc->identifier = current->fd_count;
    current->fd = krealloc(
        current->fd,
        sizeof(task_file_descriptor_t*) * (current->fd_count + 1)
    );
    current->fd[current->fd_count] = desc;
    current->fd_count++;

    return desc->identifier;
}
