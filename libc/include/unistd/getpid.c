#if !(defined(__is_libk))

#include <stdint.h>
#include <sys/syscall.h>
#include <unistd.h>

uint32_t getpid()
{
    return syscall(SYS_GETPID, 0, 0, 0);
}

#endif
