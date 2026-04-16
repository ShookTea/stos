#ifndef _SYS_IOCTL_H
#define _SYS_IOCTL_H 1
#include <sys/cdefs.h>

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

#ifdef __cplusplus
}
#endif

#endif
#endif
