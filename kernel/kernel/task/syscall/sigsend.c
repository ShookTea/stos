#include <errno.h>
#include <signal.h>
#include "kernel/memory/kmalloc.h"
#include "kernel/task/scheduler.h"
#include "kernel/task/syscall.h"
#include "kernel/task/task.h"

static void sys_sigsend_impl(
    task_t* sender __attribute__((unused)),
    task_t* receiver,
    int sig
) {
    sigaddset(&receiver->sig_pending, sig);

    if (receiver->state == TASK_BLOCKED || receiver->state == TASK_SLEEPING) {
        scheduler_move_task_to_state(receiver, TASK_WAITING);
    }
}

int sys_sigsend(int pid, int sig)
{
    if (sig < 0 || sig > 31) {
        return -EINVAL;
    }

    task_t* sender = scheduler_get_current_task();
    if (sender == NULL) {
        return -ENOTSUP;
    }

    if (pid > 0) {
        // Send signal to selected task
        task_t* receiver = task_get_task_by_pid(pid);
        if (receiver == NULL) {
            return -ESRCH;
        }
        sys_sigsend_impl(sender, receiver, sig);
        return 0;
    }

    int pgid = -1;

    if (pid == 0) {
        // Send signal to all tasks in the same process group
        pgid = sender->pgid;
    }
    else if (pid < -1) {
        // Send signal to given PGID (equal to -pid)
        pgid = -pid;
    }

    if (pgid != -1) {
        task_t** receivers = task_get_tasks_by_pgid(pgid);
        if (*receivers == NULL) {
            kfree(receivers);
            return -ESRCH;
        }
        task_t** ptr = receivers;
        while (*ptr != NULL) {
            sys_sigsend_impl(sender, *ptr, sig);
            ptr++;
        }
        kfree(receivers);
        return 0;
    }

    return -EINVAL;
}
