#include <errno.h>
#include <time.h>
#include <sys/syscall.h>

#if !(defined(__is_libk))

int nanosleep(const struct timespec* duration, struct timespec* rem)
{
    int res = syscall(SYS_SLEEP, (int)duration, (int)rem, 0);
    if (res < 0) {
        errno = -res;
        return -1;
    }
    return 0;
}

#endif
