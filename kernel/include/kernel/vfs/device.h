#ifndef INCLUDE_KERNEL_VFS_DEVICE_H
#define INCLUDE_KERNEL_VFS_DEVICE_H

#include "kernel/vfs/vfs.h"

/**
 * Mounts /dev directory.
 */
dentry_t* device_mount();

/**
 * Unmounts /dev directory and clears resources.
 */
void device_unmount();

/**
 * Pushes character to the standard input. Used by keyboard event handler,
 * as well as by terminal.c to send cursor position (^[6n action)
 */
void tty_push_char_to_buffer(char c);

#endif
