#if !(defined(__is_libk))

#include <stdint.h>
#include <sys/syscall.h>
#include <unistd.h>

int setpgid(pid_t pid, pid_t pgid)
{
    return syscall(SYS_SETPGID, pid, pgid, 0);
}

#endif
