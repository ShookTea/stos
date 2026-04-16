#include "asm/termbits.h"
#include "sys/ioctl.h"
#include "termios.h"
#if !(defined(__is_libk))

int tcgetattr(int fd, struct termios* termios_p)
{
    return ioctl_tty(fd, TCGETS, termios_p);
}

#endif
