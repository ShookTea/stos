#include "kernel/task/task.h"
#include "../syscall.h"

int sys_fork()
{
    task_t* forked = task_fork();
    // Return -1 on error
    if (forked == NULL) {
        return SYSCALL_ERROR;
    }

    // Return PID of new child to the task. The child gets that return set to
    // 0 inside task_fork() implementation.
    return forked->pid;
}
