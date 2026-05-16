#include <errno.h>
#if !(defined(__is_libk))

#include <stdint.h>
#include <sys/syscall.h>
#include <unistd.h>

void* brk(void* addr)
{
    int res = syscall(SYS_BRK, (int)addr, 0, 0);
    if (res < 0) {
        errno = -res;
        return (void*)-1;
    }
    return (void*)res;
}

#endif
