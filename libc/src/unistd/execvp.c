#if !(defined(__is_libk))

#include <sys/syscall.h>
#include <unistd.h>
#include "../stdlib/_stdlib_environ.h"

int execvp(const char* path, const char* argv[]) {
    return execvpe(path, argv, (const char**)environ);
}

#endif
