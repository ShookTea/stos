#if !(defined(__is_libk))

#include <sys/stat.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/syscall.h>

int fstat(int fd, struct stat* restrict statbuf)
{
    int res = syscall(SYS_FSTAT, fd, (int)statbuf, 0);
    if (res < 0) {
        errno = 0;
        return -1;
    }
    return 0;
}

#endif
