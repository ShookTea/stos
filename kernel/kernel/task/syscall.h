#ifndef KERNEL_TASK_SYSCALL_H
#define KERNEL_TASK_SYSCALL_H

// Syscall numbers
#define SYS_EXIT 0 // void exit(int status)
#define SYS_YIELD 1 // void yield()
#define SYS_GETPID 2 // uint32_t getpid()
#define SYS_GETPPID 3 // uint32_t getpid()
#define SYS_WRITE 4 // ssize_t write(int fd, const void *buf, size_t count)

// Syscall return values
#define SYSCALL_SUCCESS 0
#define SYSCALL_ERROR -1

/**
 * Initializes syscall handling system.
 */
void syscall_init();

#endif
