#include <errno.h>
#include "kernel/task/task.h"
#include "kernel/task/scheduler.h"
#include "../syscall.h"

int sys_readlink(const char* path, char* buf, int bufsiz)
{
    if (bufsiz < 0) {
        return -EINVAL;
    }

    task_t* current = scheduler_get_current_task();
    if (current == NULL) {
        return -ENOTSUP;
    }

    dentry_t* dentry = vfs_resolve_relative(
        current->root_node,
        current->working_directory,
        path
    );

    if (dentry == NULL) {
        return -ENOENT;
    }

    return vfs_readlink(dentry, buf, bufsiz);
}
