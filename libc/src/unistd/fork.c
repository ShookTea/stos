#if !(defined(__is_libk))

#include <sys/syscall.h>
#include <unistd.h>

pid_t fork(void)
{
    return (int)syscall(SYS_FORK, 0, 0, 0);
}

#endif
