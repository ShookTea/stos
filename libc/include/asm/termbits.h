#ifndef _ASM_TERMBITS_H
#define _ASM_TERMBITS_H 1

#include <sys/cdefs.h>
#include <stdint.h>

#if !(defined(__is_libk))

/* c_lflag bits */
// Should typed characters be displayed in stdout as well?
#define ECHO 0x01
// Running in canonical (line-buffered) mode
#define ICANON 0x02

// TC operation for reading termios structure from TTY
#define TCGETS 1
// TC operation for updating termios structure in TTY
#define TCSETS 2

#endif
#endif
