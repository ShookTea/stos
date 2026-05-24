#if !(defined(__is_libk))

#include <sys/syscall.h>
#include <unistd.h>
#include <errno.h>

int execve(
    const char* path,
    const char* argv[],
    const char* envp[]
) {
    int res = (int)syscall(SYS_EXEC, (int)path, (int)argv, (int)envp);
    if (res < 0) {
        errno = -res;
        return -1;
    }
    return 0;
}

#endif
