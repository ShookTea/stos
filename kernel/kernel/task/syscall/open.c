#include "../syscall.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/task/scheduler.h"
#include "kernel/task/task.h"
#include "kernel/vfs/vfs.h"
#include <stdint.h>

uint32_t sys_open(const char* path, uint32_t flags)
{
    task_t* current = scheduler_get_current_task();
    if (current == NULL) {
        return SYSCALL_ERROR;
    }

    vfs_node_t* node = vfs_resolve(path);
    if (node == NULL) {
        return SYSCALL_ERROR;
    }

    vfs_file_t* handler = vfs_open(node, flags);
    if (handler == NULL) {
        return SYSCALL_ERROR;
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
    desc->identifier = current->fd_count + 3; // 0-2 are reserved
    current->fd = krealloc(
        current->fd,
        sizeof(task_file_descriptor_t*) * (current->fd_count + 1)
    );
    current->fd[current->fd_count] = desc;
    current->fd_count++;

    return desc->identifier;
}
