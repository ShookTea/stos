#include <errno.h>
#include "kernel/task/task.h"
#include "../syscall.h"

int sys_fork()
{
    task_t* forked = task_fork();

    if (forked == NULL) {
        return -ENOTSUP;
    }

    // Return PID of new child to the task. The child gets that return set to
    // 0 inside task_fork() implementation.
    return forked->pid;
}
