#include <errno.h>
#include "kernel/vfs/pipe.h"
#include "kernel/task/syscall.h"
#include "kernel/task/task.h"
#include "kernel/task/scheduler.h"

int sys_pipe(int pipefd[2], int flags)
{
    task_t* current = scheduler_get_current_task();
    if (current == NULL) {
        return -ENOTSUP;
    }

    if (pipefd == NULL) {
        return -EFAULT;
    }
    return pipe_create(current, pipefd, pipefd + 1, flags);
}
