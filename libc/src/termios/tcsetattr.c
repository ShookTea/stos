#include "asm/termbits.h"
#include "sys/ioctl.h"
#include "termios.h"
#include <errno.h>
#if !(defined(__is_libk))

int tcsetattr(int fd, int optional_action, const struct termios* termios_p)
{
    if (optional_action != TCSANOW) {
        errno = EINVAL;
        return -1;
    }

    return ioctl_tty(fd, TCSETS, (struct termios*)termios_p);
}

#endif
