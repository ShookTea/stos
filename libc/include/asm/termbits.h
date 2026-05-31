#ifndef _ASM_TERMBITS_H
#define _ASM_TERMBITS_H 1

#include <sys/cdefs.h>
#include <stdint.h>

// TC operation for reading termios structure from TTY
#define TCGETS 0x01
// TC operation for updating termios structure in TTY
#define TCSETS 0x02
// Returns process group ID of the foreground process group
#define TIOCGPGRP 0x11
// Updates foreground process group ID of selected terminal
#define TIOCSPGRP 0x12

#endif
