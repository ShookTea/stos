#if !(defined(__is_libk))

#include <stdint.h>
#include <sys/syscall.h>
#include <unistd.h>

uint32_t getppid()
{
    return syscall(SYS_GETPPID, 0, 0, 0);
}

#endif
