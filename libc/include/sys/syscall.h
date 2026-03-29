#ifndef _SYS_SYSCALL_H
#define _SYS_SYSCALL_H 1
#include <sys/cdefs.h>

#if !(defined(__is_libk))

#define SYS_EXIT 0 // void exit(int status)
#define SYS_YIELD 1 // void yield()
#define SYS_GETPID 2 // uint32_t getpid()
#define SYS_GETPPID 3 // uint32_t getpid()
#define SYS_BRK 4 // uint32_t brk(void* addr)
#define SYS_WRITE 5 // ssize_t write(int fd, const void *buf, size_t count)

#ifdef __cplusplus
extern "C" {
#endif

int syscall(int number, int arg1, int arg2, int arg3);

#ifdef __cplusplus
}
#endif

#endif
#endif
