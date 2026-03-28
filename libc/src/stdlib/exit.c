#if !(defined(__is_libk))
#include <sys/syscall.h>

__attribute__((__noreturn__))
void exit(int status)
{
    syscall(SYS_EXIT, status, 0, 0);
}

#endif
