#include "kernel/task/task.h"

static void sighandler_ignore(int signum __attribute__((unused))) {}

void task_reset_sighandler(task_t* task, int signum)
{
    if (task == NULL || signum < 0 || signum > 31) {
        return;
    }

    task->sig_handlers[signum].sa_flags = 0;
    task->sig_handlers[signum].sa_handler = sighandler_ignore;
    // TODO: add valid default handlers for some signals that aren't ignored
}
