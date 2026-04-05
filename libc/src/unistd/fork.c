#if !(defined(__is_libk))

#include <sys/syscall.h>
#include <unistd.h>

int fork()
{
    return (int)syscall(SYS_FORK, 0, 0, 0);
}

#endif
