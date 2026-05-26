#if !(defined(__is_libk))

#include <unistd.h>
#include <errno.h>
#include <sys/syscall.h>

int dup(int oldfd)
{
    if (oldfd < 0) {
        errno = EINVAL;
        return -1;
    }

    // -1 is treated by SYS_DUP as flag for "dup() used"
    int res = syscall(SYS_DUP, oldfd, -1, 0);
    if (res < 0) {
        errno = -res;
        return -1;
    }

    return res;
}

#endif
