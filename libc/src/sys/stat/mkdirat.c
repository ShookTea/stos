#if !(defined(__is_libk))

#include <sys/stat.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/syscall.h>

int mkdirat(int dirfd, const char *path, mode_t mode)
{
    int res = syscall(SYS_MKDIR, dirfd, (int)path, mode);
    if (res < 0) {
        errno = 0;
        return -1;
    }
    return 0;
}

#endif
