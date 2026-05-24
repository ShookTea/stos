#if !(defined(__is_libk))

#include <sys/syscall.h>
#include <unistd.h>

int execvpe(const char* path, const char* argv[], const char* envp[]) {
    return execve(path, argv, envp);
}

#endif
