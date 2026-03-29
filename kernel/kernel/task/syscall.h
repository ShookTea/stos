#ifndef KERNEL_TASK_SYSCALL_H
#define KERNEL_TASK_SYSCALL_H

#include <stdint.h>
#include <stddef.h>

// Syscall numbers
#define SYS_EXIT 0
uint32_t sys_exit(int status_code);

#define SYS_YIELD 1
uint32_t sys_yield();

#define SYS_GETPID 2
uint32_t sys_getpid();

#define SYS_GETPPID 3
uint32_t sys_getppid();

#define SYS_WRITE 4
int sys_write(int fd, const void* buf, size_t count);

// Syscall return values
#define SYSCALL_SUCCESS 0
#define SYSCALL_ERROR -1

/**
 * Initializes syscall handling system.
 */
void syscall_init();

#endif
