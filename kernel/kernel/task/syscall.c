#include "syscall.h"
#include <kernel/memory/vmm.h>
#include <kernel/terminal.h>
#include <kernel/task/scheduler.h>
#include <kernel/task/task.h>
#include <stdint.h>
#include <kernel/task/syscall_handler.h>
#include <stdio.h>

static uint32_t sys_exit(int status_code)
{
    task_exit(status_code);
    return SYSCALL_SUCCESS; // Never reached - task_exit is noreturn
}

static int sys_write(int fd, const void* buf, size_t count)
{
    // For now only support stdout (fd=1)
    if (fd != 1) {
        return -1;
    }

    // Validate user pointer is in user space
    if ((uint32_t)buf < VMM_USER_START || (uint32_t)buf >= VMM_USER_END) {
        return -1;
    }

    // Write to terminal
    const char* str = (const char*)buf;
    for (size_t i = 0; i < count; i++) {
        terminal_write_char(str[i]);
    }

    return (int)count;
}

static uint32_t sys_getpid()
{
    task_t* current = scheduler_get_current_task();
    return current ? current->pid : 0;
}

static uint32_t sys_getppid()
{
    task_t* current = scheduler_get_current_task();
    if (current == NULL || current->parent == NULL) {
        return 0;
    }
    return current->parent->pid;
}

static uint32_t sys_yield()
{
    scheduler_yield();
    return SYSCALL_SUCCESS; // Never reached - scheduler_yield is noreturn
}

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
