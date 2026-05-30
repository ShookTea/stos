#include <errno.h>
#include <string.h>
#include "kernel/task/scheduler.h"
#include "kernel/task/syscall.h"
#include "kernel/task/task.h"
#include "signal.h"

int sys_sigact(int sig, const struct sigaction* act, struct sigaction* oldact)
{
    task_t* current = scheduler_get_current_task();
    if (current == NULL) {
        return -ENOTSUP;
        return -ENOTSUP;
    }

    if (oldact != NULL) {
        memcpy(oldact, &current->sig_handlers[sig], sizeof(struct sigaction));
    }

    if (act != NULL) {
        // TODO: detect if given signum can be overwritten
        // TODO: detect `SIG_DFL` and `SIG_IGN`
        memcpy(&current->sig_handlers[sig], act, sizeof(struct sigaction));
    }

    return 0;
}
