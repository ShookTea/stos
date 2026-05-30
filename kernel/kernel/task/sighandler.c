#include "kernel/task/task.h"
#include <signal.h>

void task_reset_sighandler(task_t* task, int signum)
{
    if (task == NULL || signum < 0 || signum > 31) {
        return;
    }

    task->sig_handlers[signum].sa_flags = 0;
    task->sig_handlers[signum].sa_handler = SIG_DFL;
}
