#include "syscall.h"
#include <kernel/memory/vmm.h>
#include <kernel/terminal.h>
#include <kernel/task/scheduler.h>
#include <kernel/task/task.h>
#include <stdint.h>
#include <kernel/task/syscall_handler.h>
#include <stdio.h>

static uint32_t syscall_int_handler(
    uint32_t syscall_num,
    uint32_t arg1,
    uint32_t arg2,
    uint32_t arg3
) {
    switch (syscall_num) {
        case SYS_EXIT: return sys_exit((int)arg1);
        case SYS_YIELD: return sys_yield();
        case SYS_GETPID: return sys_getpid();
        case SYS_GETPPID: return sys_getppid();
        case SYS_BRK: return sys_brk(arg1);
        case SYS_WRITE:
            return sys_write((int)arg1, (const void*)arg2, (size_t)arg3);
        default:
            printf("Unknown syscall: %d\n", syscall_num);
            return SYSCALL_ERROR;
    }
}

void syscall_init()
{
    syscall_handler_register(syscall_int_handler);
}
