#if !(defined(__is_libk))

#include <sys/wait.h>
#include <sys/syscall.h>

int waitpid(int pid, int* status_code, int options)
{
    return (int)syscall(SYS_WAIT, pid, (int)status_code, options);
}

#endif
