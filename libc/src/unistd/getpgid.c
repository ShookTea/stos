#if !(defined(__is_libk))

#include <errno.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <unistd.h>

pid_t getpgid(pid_t pid)
{
    int res = syscall(SYS_GETPGID, pid, 0, 0);
    if (res < 0) {
        errno = -res;
        return -1;
    }

    return res;
}

#endif
