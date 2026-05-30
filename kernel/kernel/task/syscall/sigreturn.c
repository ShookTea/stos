#include <errno.h>
#include "kernel/task/scheduler.h"
#include "kernel/task/syscall.h"
#include "kernel/task/task.h"

int sys_sigreturn(void)
{
    task_t* task = scheduler_get_current_task();
    if (task == NULL) {
        return -ENOTSUP;
    }
    return task_sigreturn(task);
}
