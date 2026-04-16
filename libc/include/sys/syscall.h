#ifndef _SYS_SYSCALL_H
#define _SYS_SYSCALL_H 1
#include <sys/cdefs.h>

#if !(defined(__is_libk))

#define SYS_EXIT    0x00 // void exit(int status)
#define SYS_YIELD   0x01 // void yield()
#define SYS_GETPID  0x02 // uint32_t getpid()
#define SYS_GETPPID 0x03 // uint32_t getpid()
#define SYS_FORK    0x04 // int fork()
#define SYS_WAIT    0x05 // int waitpid(int pid, int* status, int options)
#define SYS_EXEC    0x06 // int execve(const char*, const char*[], const char*[])
#define SYS_GETCWD  0x07 // char* getcwd(char buf[], size_t size)
#define SYS_CHDIR   0x08 // int chdir(const char* path)

#define SYS_OPEN    0x10 // int open(const char* path, int flags)
#define SYS_CLOSE   0x11 // int close(int fd);
#define SYS_WRITE   0x12 // int write(int fd, const void* buf, size_t count)
#define SYS_READ    0x13 // int read(int fd, void* buf, size_t count)
#define SYS_IOCTL   0x14 // int ioctl(int fd, int op, void* arg)

#define SYS_BRK     0x20 // uint32_t brk(void* addr)

#ifdef __cplusplus
extern "C" {
#endif

int syscall(int number, int arg1, int arg2, int arg3);

#ifdef __cplusplus
}
#endif

#endif
#endif
