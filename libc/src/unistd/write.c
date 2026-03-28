#if !(defined(__is_libk))

#include <sys/syscall.h>
#include <unistd.h>
#include <stddef.h>

int write(int fd, const void* buffer, size_t count)
{
    return syscall(SYS_WRITE, fd, (int)buffer, count);
}

#endif
