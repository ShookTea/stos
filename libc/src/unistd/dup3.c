#if !(defined(__is_libk))

#include <unistd.h>
#include <errno.h>
#include <sys/syscall.h>

int dup3(int oldfd, int newfd, int flags)
{
    if (newfd == oldfd || oldfd < 0 || newfd < 0) {
        errno = EINVAL;
        return -1;
    }

    if (flags != 0) {
        errno = EINVAL;
        return -1;
    }

    int res = syscall(SYS_DUP, oldfd, newfd, flags);
    if (res < 0) {
        errno = -res;
        return -1;
    }

    return res;
}

#endif
