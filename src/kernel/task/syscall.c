#include "syscall.h"
#include <stdint.h>
#include <kernel/task/syscall_handler.h>
#include <stdio.h>

static uint32_t syscall_int_handler(
    uint32_t arg0,
    uint32_t arg1,
    uint32_t arg2,
    uint32_t arg3
) {
    printf(
        "Syscall received, arg0=%d arg1=%d arg2=%d arg3=%d\n",
        arg0,
        arg1,
        arg2,
        arg3
    );
    return arg0 + arg1 + arg2 + arg3;
}

void syscall_init()
{
    syscall_handler_register(syscall_int_handler);
}
