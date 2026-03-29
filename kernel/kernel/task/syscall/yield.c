#include <stdint.h>
#include "../syscall.h"
#include <kernel/task/scheduler.h>

uint32_t sys_yield()
{
    scheduler_yield();
    return SYSCALL_SUCCESS;
}
