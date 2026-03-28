#if !(defined(__is_libk))

#include <sys/syscall.h>

int sched_yield()
{
    return syscall(SYS_YIELD, 0, 0, 0);
}

#endif
