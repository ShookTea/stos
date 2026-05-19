#include <stdint.h>
#include "kernel/task/syscall.h"
#include <kernel/task/scheduler.h>

uint32_t sys_yield()
{
    scheduler_yield();
    return 0;
}
