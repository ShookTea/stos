#ifndef _SYS_REBOOT_H
#define _SYS_REBOOT_H 1
#include <sys/cdefs.h>

/**
 * Turn off the computer.
 */
#define RB_POWER_OFF 1

/**
 * Restarts the computer.
 */
#define RB_AUTOBOOT 2

/**
 * Requests reboot or shutdown. `op` is one of `RB_` values. On success it
 * doesn't return; on failure -1 is returned, and `errno` is set to error code.
 */
int reboot(int op);

#endif
