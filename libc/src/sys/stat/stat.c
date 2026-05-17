#if !(defined(__is_libk))

#include <sys/stat.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/syscall.h>

int stat(const char* restrict path, struct stat* restrict statbuf)
{
    int res = syscall(SYS_STAT, (int)path, (int)statbuf, false);
    if (res < 0) {
        errno = 0;
        return -1;
    }
    return 0;
}

#endif
