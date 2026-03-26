#ifndef KERNEL_TASK_SYSCALL_H
#define KERNEL_TASK_SYSCALL_H

// Syscall numbers
#define SYS_EXIT 1 // void exit(int status)
#define SYS_WRITE 4 // ssize_t write(int fd, const void *buf, size_t count)
#define SYS_GETPID 20 // uint32_t getpid()
#define SYS_YIELD 24 // void yield()

// Syscall return values
#define SYSCALL_SUCCESS 0
#define SYSCALL_ERROR -1

/**
 * Initializes syscall handling system.
 */
void syscall_init();

#endif
