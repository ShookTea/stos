#include <errno.h>
#include "kernel/task/scheduler.h"
#include "kernel/task/syscall.h"
#include "kernel/task/task.h"

int sys_dup(int oldfd, int newfd, int flags)
{
    if (flags != 0 || oldfd < 0) {
        return -EINVAL;
    }

    task_t* current = scheduler_get_current_task();
    if (current == NULL) {
        return -ENOTSUP;
    }

    return -ENOTSUP;

    if (newfd == -1) {
        // Just clone
    }
}
