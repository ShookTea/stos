#if !(defined(__is_libk))

#include <sys/syscall.h>
#include <unistd.h>
#include <stddef.h>

int read(int fd, void* buffer, size_t count)
{
    return syscall(SYS_READ, fd, (int)buffer, count);
}

#endif
