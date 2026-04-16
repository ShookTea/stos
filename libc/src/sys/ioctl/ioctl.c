#if !(defined(__is_libk))
#include <sys/syscall.h>
#include <sys/ioctl.h>

int ioctl(int fd, int op, void* arg)
{
    return syscall(SYS_IOCTL, fd, op, (int)arg);
}

#endif
