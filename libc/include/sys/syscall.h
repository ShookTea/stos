#ifndef _SYS_SYSCALL_H
#define _SYS_SYSCALL_H 1
#include <sys/cdefs.h>

#if !(defined(__is_libk))

#define SYS_EXIT 1 // void exit(int status)
#define SYS_WRITE 4 // ssize_t write(int fd, const void *buf, size_t count)
#define SYS_GETPID 20 // uint32_t getpid()
#define SYS_YIELD 24 // void yield()

#ifdef __cplusplus
extern "C" {
#endif

int syscall(int number, int arg1, int arg2, int arg3);

#ifdef __cplusplus
}
#endif

#endif
#endif
