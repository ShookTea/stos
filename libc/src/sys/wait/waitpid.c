#if !(defined(__is_libk))

#include <errno.h>
#include <sys/wait.h>
#include <sys/syscall.h>

int waitpid(int pid, int* status_code, int options)
{
    int res = syscall(SYS_WAIT, pid, (int)status_code, options);
    if (res < 0) {
        errno = -res;
        return -1;
    }
    return res;
}

#endif
