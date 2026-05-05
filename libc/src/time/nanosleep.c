#include <time.h>
#include <sys/syscall.h>

#if !(defined(__is_libk))

int nanosleep(const struct timespec* duration, struct timespec* rem)
{
    return syscall(SYS_SLEEP, (int)duration, (int)rem, 0);
}

#endif
