#ifndef KERNEL_TASK_SYSCALL_HANDLER_H
#define KERNEL_TASK_SYSCALL_HANDLER_H

#include <stdint.h>

/**
 * Syscall interrupt handler, receiving four possible arguments.
 */
typedef uint32_t (*syscall_int_handler_t)(
    uint32_t,
    uint32_t,
    uint32_t,
    uint32_t
);

/**
 * Register given int handler to be called when syscall interrupt is received.
 */
void syscall_handler_register(syscall_int_handler_t handler);

#endif
