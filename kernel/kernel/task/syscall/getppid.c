#include <kernel/task/scheduler.h>
#include <stdint.h>
#include "../syscall.h"

uint32_t sys_getppid()
{
    task_t* current = scheduler_get_current_task();
    if (current == NULL || current->parent == NULL) {
        return 0;
    }
    return current->parent->pid;
}
