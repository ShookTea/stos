#include <kernel/task/scheduler.h>
#include <stdint.h>
#include "kernel/task/syscall.h"

pid_t sys_getpid()
{
    task_t* current = scheduler_get_current_task();
    return current ? current->pid : 0;
}
