#if !(defined(__is_libk))

#include <sys/syscall.h>
#include <signal.h>
#include <errno.h>

int kill(int pid, int sig) {
    if (sig < 0 || sig > 31) {
        errno = EINVAL;
        return -1;
    }
    int res = syscall(SYS_SIGSEND, pid, sig, 0);
    if (res < 0) {
        errno = -res;
        return -1;
    }
    return 0;
}

#endif
