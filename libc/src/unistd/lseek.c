#if !(defined(__is_libk))

#include <sys/syscall.h>
#include <unistd.h>
#include <stddef.h>

int lseek(int fd, int offset, int whence)
{
    return syscall(SYS_LSEEK, fd, offset, whence);
}

#endif
