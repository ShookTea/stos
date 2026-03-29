#include <kernel/task/scheduler.h>
#include <stdint.h>
#include "../syscall.h"

uint32_t sys_getpid()
{
    task_t* current = scheduler_get_current_task();
    return current ? current->pid : 0;
}
