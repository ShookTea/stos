#if !(defined(__is_libk))

#include <sys/syscall.h>
#include <fcntl.h>
#include <stdbool.h>

int close(int fd)
{
    int res = syscall(SYS_CLOSE, fd, 0, 0);
    return res;
}

#endif
