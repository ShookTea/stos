#ifndef _SYS_SYSCALL_H
#define _SYS_SYSCALL_H 1
#include <sys/cdefs.h>

// Process management
#define SYS_EXIT        0x00 // void exit(int status)
#define SYS_YIELD       0x01 // void yield()
#define SYS_GETPID      0x02 // pid_t getpid()
#define SYS_GETPPID     0x03 // pid_t getpid()
#define SYS_GETPGID     0x04 // pid_t getpgid(pid_t pid)
#define SYS_SETPGID     0x05 // int setpgid(pid_t pid, pid_t pgid)
#define SYS_FORK        0x06 // int fork()
#define SYS_WAIT        0x07 // int waitpid(int pid, int* status, int options)
#define SYS_EXEC        0x08 // int execve(const char*, const char*[], const char*[])
#define SYS_GETCWD      0x09 // char* getcwd(char buf[], size_t size)
#define SYS_CHDIR       0x0A // int chdir(const char* path)
#define SYS_SLEEP       0x0B // int nanosleep(const struct timespec* duration, struct timespec* rem)

// File management
#define SYS_OPEN        0x10 // int open(const char* path, int flags)
#define SYS_CLOSE       0x11 // int close(int fd);
#define SYS_WRITE       0x12 // int write(int fd, const void* buf, size_t count)
#define SYS_READ        0x13 // int read(int fd, void* buf, size_t count)
#define SYS_IOCTL       0x14 // int ioctl(int fd, int op, void* arg)
#define SYS_LSEEK       0x15 // int lseek(int fd, int offset, int whence)
#define SYS_STAT        0x16 // int stat(const char* path, struct stat* statbuf)
#define SYS_FSTAT       0x17 // int fstat(int fd, struct stat* statbuf)
#define SYS_READLINK    0x18 // size_t readlink(const char* path, char* buf, size_t bufsiz);
#define SYS_GETDENTS    0x19 // no wrapper in libc
#define SYS_DUP         0x1A // int dup3(int oldfd, int newfd, int flags);

// Memory operations
#define SYS_BRK         0x20 // uint32_t brk(void* addr)

// Time operations
#define SYS_UNIXTIME    0x30 // time_t time(time_t* tloc)

// Inter-process commmunication
#define SYS_SIGACT      0x40 // int sigaction(int, const struct sigaction*, struct sigaction*)
#define SYS_SIGSEND     0x41 // int kill(int pid, int sig)
#define SYS_SIGRETURN   0x42 // no wrapper in libc - called by trampoline only
#define SYS_PIPE        0x43 // int pipe2(int pipefd[2], int flags);

// System management
#define SYS_REBOOT      0x50 // int reboot(int op)

#if !(defined(__is_libk))

#ifdef __cplusplus
extern "C" {
#endif

int syscall(int number, int arg1, int arg2, int arg3);

#ifdef __cplusplus
}
#endif

#endif
#endif
