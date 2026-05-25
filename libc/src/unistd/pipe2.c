#include <errno.h>
#if !(defined(__is_libk))

#include <unistd.h>
#include <sys/syscall.h>

int pipe2(int pipefd[2], int flags)
{
    if (flags != 0) {
        errno = EINVAL;
        return -1;
    }
    if (pipefd == NULL) {
        errno = EFAULT;
        return -1;
    }

    int res = syscall(SYS_PIPE, (int)pipefd, flags, 0);
    if (res < 0) {
        errno = -res;
        return -1;
    }

    return 0;
}

#endif
