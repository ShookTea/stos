#include "kernel/task/syscall.h"
#include <errno.h>
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

    int err_from_open = 0;
    vfs_file_t* handler = vfs_open(node, flags, &err_from_open);
    if (handler == NULL) {
        return err_from_open == 0 ? -EIO : -err_from_open;
    }

    task_file_descriptor_t* desc = task_add_fd(current, handler);
    if (desc == NULL) {
        return -EIO;
    }
    return desc->identifier;
}
