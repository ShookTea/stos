#include <kernel/task/scheduler.h>
#include <stdint.h>
#include "kernel/task/syscall.h"

pid_t sys_getppid()
{
    task_t* current = scheduler_get_current_task();
    if (current == NULL || current->parent == NULL) {
        return 0;
    }
    return current->parent->pid;
}
