#include <stdint.h>
#include <errno.h>
#include "kernel/task/syscall.h"
#include "kernel/task/scheduler.h"
#include "kernel/task/task.h"
#include "kernel/vfs/vfs.h"
#include "kernel/vfs/path.h"

int sys_chdir(const char* path)
{
    task_t* task = scheduler_get_current_task();
    if (task == NULL) {
        return -ENOTSUP;
    }

    dentry_t* new_workdir = path_resolve_relative(
        task->root_node,
        task->working_directory,
        path
    );

    if (new_workdir == NULL) {
        return -ENOENT;
    }

    task->working_directory = new_workdir;
    return 0;
}
