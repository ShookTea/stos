#if !(defined(__is_libk))

#include <sys/syscall.h>
#include <unistd.h>

int execve(
    const char* path,
    const char* argv[] __attribute__((unused)),
    const char* envp[] __attribute__((unused))
) {
    return (int)syscall(SYS_EXEC, (int)path, 0, 0);
}

#endif
