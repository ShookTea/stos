#if !(defined(__is_libk))

#include <stdint.h>
#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

int setpgid(pid_t pid, pid_t pgid)
{
    int res = syscall(SYS_SETPGID, pid, pgid, 0);
    if (res < 0) {
        errno = -res;
        return -1;
    }
    return 0;
}

#endif
