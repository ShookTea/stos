#include <kernel/task/scheduler.h>
#include <stdint.h>
#include "errno.h"
#include "kernel/task/syscall.h"
#include "kernel/task/task.h"

pid_t sys_getpgid(pid_t pid)
{
    task_t* to_check;
    if (pid == 0) {
        to_check = scheduler_get_current_task();
        if (to_check == NULL) {
            return -ENOTSUP;
        }
    } else {
        to_check = task_get_task_by_pid(pid);
        if (to_check == NULL) {
            return -ESRCH;
        }
    }

    return to_check->pgid;
}
