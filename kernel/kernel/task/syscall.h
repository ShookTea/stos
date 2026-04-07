#ifndef KERNEL_TASK_SYSCALL_H
#define KERNEL_TASK_SYSCALL_H

#include <stdint.h>
#include <stddef.h>

// Syscall numbers
#define SYS_EXIT 0x00
uint32_t sys_exit(int status_code);

#define SYS_YIELD 0x01
uint32_t sys_yield();

#define SYS_GETPID 0x02
uint32_t sys_getpid();

#define SYS_GETPPID 0x03
uint32_t sys_getppid();

#define SYS_BRK 0x04
uint32_t sys_brk(uint32_t addr);

#define SYS_FORK 0x05
int sys_fork();

#define SYS_WAIT 0x06
int sys_wait(int pid, int* status_code, int options);

#define SYS_OPEN 0x10
uint32_t sys_open(const char* path, uint32_t flags);

#define SYS_CLOSE 0x11
int sys_close(int fd);

#define SYS_WRITE 0x12
int sys_write(int fd, const void* buf, size_t count);

#define SYS_READ 0x13
int sys_read(int fd, void* buf, size_t count);

// Syscall return values
#define SYSCALL_SUCCESS 0
#define SYSCALL_ERROR -1

// Syscall options
#define SYS_WAIT_OPT_NO_HANG 1

/**
 * Initializes syscall handling system.
 */
void syscall_init();

#endif
