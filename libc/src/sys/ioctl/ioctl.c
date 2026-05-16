#if !(defined(__is_libk))

#include <errno.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>

int ioctl(int fd, int op, void* arg)
{
    int res = syscall(SYS_IOCTL, fd, op, (int)arg);
    if (res < 0) {
        errno = -res;
        return -1;
    }
    return res;
}

#endif
