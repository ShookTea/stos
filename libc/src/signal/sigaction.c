#if !(defined(__is_libk))

#include <sys/syscall.h>
#include <signal.h>
#include <errno.h>

int sigaction(
    int signum,
    const struct sigaction* restrict act,
    struct sigaction* restrict oldact
) {
    int res = syscall(SYS_SIGACT, signum, (int)act, (int)oldact);
    if (res < 0) {
        errno = -res;
        return -1;
    }
    return 0;
}

#endif
