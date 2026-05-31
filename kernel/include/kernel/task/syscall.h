#ifndef KERNEL_TASK_SYSCALL_H
#define KERNEL_TASK_SYSCALL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include "kernel/task/task.h"

uint32_t sys_exit(int status_code);
uint32_t sys_yield();
pid_t sys_getpid();
pid_t sys_getppid();
pid_t sys_getpgid(pid_t pid);
int sys_setpgid(pid_t pid, pid_t pgid);
int sys_fork();
int sys_wait(int pid, int* status_code, int options);
int sys_exec(const char* path, const char** argv, const char** envp);
char* sys_getcwd(char* buf, size_t size);
int sys_chdir(const char* path);
int sys_sleep(const struct timespec* duration, struct timespec* rem);
uint32_t sys_open(const char* path, uint32_t flags);
int sys_close(int fd);
int sys_write(int fd, const void* buf, size_t count);
int sys_read(int fd, void* buf, size_t count);
int sys_ioctl(int fd, int op, void* arg);
int sys_lseek(int fd, int offset, int whence);
int sys_stat(const char* path, struct stat* stat, bool link_ignore);
int sys_fstat(int fd, struct stat* stat);
int sys_readlink(const char* path, char* buf, int bufsiz);
int sys_getdents(int fd, struct dirent* dir, int count);
int sys_dup(int oldfd, int newfd, int flags);
int sys_brk(uint32_t addr);
int sys_time(time_t* result_ptr);
int sys_sigact(int sig, const struct sigaction* act, struct sigaction* oldact);
int sys_sigsend(int pid, int sig);
int sys_sigreturn(void);
int sys_pipe(int pipefd[2], int flags);

/**
 * Initializes syscall handling system.
 */
void syscall_init();

/**
 * Returns valid descriptor by FD value
 */
task_file_descriptor_t* syscall_get_descriptor_by_fd(int fd);

#endif
