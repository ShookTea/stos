#include <stdbool.h>
#include <errno.h>
#include "kernel/vfs/vfs.h"
#include "kernel/task/task.h"
#include "kernel/task/scheduler.h"

int sys_stat(const char* path, struct stat* stat, bool link_ignore)
{
    task_t* current = scheduler_get_current_task();
    if (current == NULL) {
        return -ENOTSUP;
    }

    dentry_t* node = NULL;
    if (link_ignore) {
        node = vfs_resolve_relative_no_follow(
            current->root_node,
            current->working_directory,
            path
        );
    } else {
        node = vfs_resolve_relative(
            current->root_node,
            current->working_directory,
            path
        );
    }
    if (node == NULL) {
        return -ENOENT;
    }

    return -vfs_stat(node, stat);
}
