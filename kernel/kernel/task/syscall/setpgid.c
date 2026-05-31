#include <kernel/task/scheduler.h>
#include <stdint.h>
#include "errno.h"
#include "kernel/task/syscall.h"
#include "kernel/task/task.h"

int sys_setpgid(pid_t pid, pid_t pgid)
{
    task_t* to_update;
    if (pid == 0) {
        to_update = scheduler_get_current_task();
        if (to_update == NULL) {
            return -ENOTSUP;
        }
    } else {
        to_update = task_get_task_by_pid(pid);
        if (to_update == NULL) {
            return -ESRCH;
        }
    }

    to_update->pgid = pgid > 0 ? pgid : to_update->pid;
    return 0;
}
