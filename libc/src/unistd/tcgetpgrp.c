#if !(defined(__is_libk))

#include <sys/ioctl.h>
#include <unistd.h>

pid_t tcgetpgrp(int fd)
{
    pid_t pgid = 0;
    int res = ioctl(fd, TIOCGPGRP, &pgid);
    if (res < 0) {
        return -1;
    }
    return pgid;
}

#endif
