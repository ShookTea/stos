#if !(defined(__is_libk))

#include <stdint.h>
#include <sys/syscall.h>
#include <unistd.h>

void* brk(void* addr)
{
    return (void*)syscall(SYS_BRK, (int)addr, 0, 0);
}

#endif
