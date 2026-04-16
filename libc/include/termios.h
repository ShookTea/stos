#ifndef _TERMIOS_H
#define _TERMIOS_H 1

#include <sys/cdefs.h>
#include <stdint.h>

#if !(defined(__is_libk))

struct termios {
    uint32_t c_iflag;
    uint32_t c_oflag;
    uint32_t c_cflag;
    uint32_t c_lflag;
    unsigned char c_line;
    unsigned char c_cc[10];
};

/* c_lflag bits */
// Should typed characters be displayed in stdout as well?
#define ECHO 0x01
// Running in canonical (line-buffered) mode
#define ICANON 0x02

#define TCSANOW 1

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Sets parameters of TTY referred to by open file descriptor `fd`.
 * `optional_action` defines the way ioctl is updated:
 * - `TCSANOW` - the change will occur immediately.
 *
 * It will return 0 on success.
 */
int tcsetattr(int fd, int optional_action, const struct termios* termios_p);

/**
 * Loads parameters of TTY refrerred to by open file descriptor `fd`.
 *
 * It will return 0 on success.
 */
int tcgetattr(int fd, struct termios* termios_p);

#ifdef __cplusplus
}
#endif

#endif
#endif
