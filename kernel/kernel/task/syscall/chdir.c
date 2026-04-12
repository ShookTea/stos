#include "../syscall.h"
#include "kernel/task/scheduler.h"
#include "kernel/task/task.h"
#include "kernel/vfs/vfs.h"
#include <stdint.h>

int sys_chdir(const char* path)
{
    task_t* task = scheduler_get_current_task();
    if (task == NULL) {
        return SYSCALL_ERROR;
    }

    dentry_t* new_workdir = vfs_resolve_relative(
        task->root_node,
        task->working_directory,
        path
    );

    if (new_workdir == NULL) {
        return SYSCALL_ERROR;
    }

    task->working_directory = new_workdir;
    return SYSCALL_SUCCESS;
}
