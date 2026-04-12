#if !(defined(__is_libk))

#include <sys/syscall.h>
#include <unistd.h>
#include "../stdlib/_stdlib_environ.h"

int execv(const char* path, const char* argv[]) {
    return execve(path, argv, (const char**)environ);
}

#endif
