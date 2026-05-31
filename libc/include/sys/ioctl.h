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
 *
 * On failure returns -1 and sets `errno` to one of following values:
 * - EBADF - if given file descriptor is invalid
 * - ENOTTY - if given operation is not valid for given descriptor
 */
int ioctl(int fd, int op, void* arg);

/**
 * Run ioctl call for a TTY file. Valid `op` values are TC* operators described
 * in the `asm/termbits.h` file.
 *
 * On failure returns -1 and sets `errno` to one of following values:
 * - EBADF - if given file descriptor is invalid
 * - ENOTTY - if given operation is not valid for given descriptor
 */
inline int ioctl_tty(int fd, int op, struct termios* arg)
{
    // ioctl handles setting errno
    return ioctl(fd, op, arg);
}


#ifdef __cplusplus
}
#endif

#endif
#endif
