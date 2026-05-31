#if !(defined(__is_libk))

#include <stdint.h>
#include <sys/syscall.h>
#include <unistd.h>

pid_t getpgid(pid_t pid)
{
    return syscall(SYS_GETPGID, pid, 0, 0);
}

#endif
