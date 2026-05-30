#include <errno.h>
#include "kernel/task/scheduler.h"
#include "kernel/task/syscall.h"
#include "kernel/task/task.h"
#include "kernel/task/wait.h"

int sys_sigsend(int pid, int sig __attribute__((unused)))
{
    if (sig < 0 || sig > 31) {
        return -EINVAL;
    }

    task_t* sender = scheduler_get_current_task();
    if (sender == NULL) {
        return -ENOTSUP;
    }

    task_t* receiver = task_get_task_by_pid(pid);
    if (receiver == NULL) {
        return -ESRCH;
    }

    return -ENOTSUP; // TODO: send signal
}
