#include <stdint.h>
#include "kernel/task/syscall.h"
#include <kernel/task/task.h>

uint32_t sys_exit(int status_code)
{
    task_exit(status_code);
    return 0; // Never reached - task_exit is noreturn
}
