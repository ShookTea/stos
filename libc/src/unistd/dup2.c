#if !(defined(__is_libk))

#include <unistd.h>
#include <errno.h>
#include <sys/syscall.h>

int dup2(int oldfd, int newfd)
{
    if (oldfd < 0 || newfd < 0) {
        errno = EINVAL;
        return -1;
    }
    if (newfd == oldfd) {
        return newfd;
    }

    int res = syscall(SYS_DUP, oldfd, newfd, 0);
    if (res < 0) {
        errno = -res;
        return -1;
    }

    return res;
}

#endif
