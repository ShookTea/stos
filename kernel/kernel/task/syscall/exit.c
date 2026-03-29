#include <stdint.h>
#include "../syscall.h"
#include <kernel/task/task.h>

uint32_t sys_exit(int status_code)
{
    task_exit(status_code);
    return SYSCALL_SUCCESS; // Never reached - task_exit is noreturn
}
