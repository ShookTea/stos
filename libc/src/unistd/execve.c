#if !(defined(__is_libk))

#include <sys/syscall.h>
#include <unistd.h>

int execve(
    const char* path,
    const char* argv[],
    const char* envp[]
) {
    return (int)syscall(SYS_EXEC, (int)path, (int)argv, (int)envp);
}

#endif
