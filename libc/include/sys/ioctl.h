#ifndef _SYS_IOCTL_H
#define _SYS_IOCTL_H 1
#include <sys/cdefs.h>
#include <asm/termbits.h>
#include <termios.h>

#if !(defined(__is_libk))

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Manipulates parameters of given special file.
 *
 * - `fd` must be an open file descriptor
 * - `op` is one of commands accepted by given file
 * - `arg` is a pointer containing arguments.
 */
int ioctl(int fd, int op, void* arg);

/**
 * Run ioctl call for a TTY file. Valid `op` values are TC* operators described
 * in the `asm/termbits.h` file.
 */
inline int ioctl_tty(int fd, int op, struct termios* arg)
{
    return ioctl(fd, op, arg);
}


#ifdef __cplusplus
}
#endif

#endif
#endif
