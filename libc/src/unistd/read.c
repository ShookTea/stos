#if !(defined(__is_libk))

#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stddef.h>

int read(int fd, void* buffer, size_t count)
{
    int res = syscall(SYS_READ, fd, (int)buffer, count);
    if (res < 0) {
        errno = -res;
        return -1;
    }
    return res;
}

#endif
