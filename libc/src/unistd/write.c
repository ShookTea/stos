#if !(defined(__is_libk))

#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stddef.h>

int write(int fd, const void* buffer, size_t count)
{
    int res = syscall(SYS_WRITE, fd, (int)buffer, count);
    if (res < 0) {
        errno = -res;
        return -1;
    }
    return res;
}

#endif
