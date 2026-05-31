#if !(defined(__is_libk))

#include <sys/ioctl.h>
#include <unistd.h>

int tcsetpgrp(int fd, pid_t pgid)
{
    return ioctl(fd, TIOCSPGRP, &pgid);
}

#endif
