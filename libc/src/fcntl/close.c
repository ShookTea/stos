#if !(defined(__is_libk))

#include <errno.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <stdbool.h>

int close(int fd)
{
    int res = syscall(SYS_CLOSE, fd, 0, 0);
    if (res < 0) {
        errno = -res;
        return -1;
    }
    return res;
}

#endif
