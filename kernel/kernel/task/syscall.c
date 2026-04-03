#include "syscall.h"
#include <kernel/memory/vmm.h>
#include <kernel/terminal.h>
#include <kernel/task/scheduler.h>
#include <kernel/task/task.h>
#include <stdint.h>
#include <kernel/task/syscall_handler.h>
#include <stdio.h>
#include "kernel/vfs/vfs.h"


#define ptr_valid(ptr) \
    ((uint32_t)ptr >= VMM_USER_START && (uint32_t)ptr <= VMM_USER_END)
#define assert_ptr(ptr) if (!ptr_valid(ptr)) { return SYSCALL_ERROR; }
#define assert_range(ptr,size) if (\
    !ptr_valid(ptr) \
    || !ptr_valid((uint32_t)ptr + (uint32_t)size) \
    || (((uint32_t)ptr + (uint32_t)size) < ((uint32_t)ptr)) \
    ) { return SYSCALL_ERROR; }

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
        case SYS_OPEN: {
            const char* path = (const char*)arg1;
            assert_range(path, VFS_MAX_PATH_LENGTH);
            return sys_open(path, arg2);
        }
        case SYS_CLOSE: return sys_close((int)arg1);
        case SYS_WRITE: {
            const void* buf = (const void*)arg2;
            assert_range(buf, arg3);
            return sys_write((int)arg1, buf, (size_t)arg3);
        }
        case SYS_READ: {
            void* buf = (void*)arg2;
            assert_range(buf, arg3);
            return sys_read((int)arg1, buf, (size_t)arg3);
        }
        default:
            printf("Unknown syscall: %d\n", syscall_num);
            return SYSCALL_ERROR;
    }
}

void syscall_init()
{
    syscall_handler_register(syscall_int_handler);
}
