#ifndef INCLUDE_KERNEL_VFS_DEVICE_H
#define INCLUDE_KERNEL_VFS_DEVICE_H

#include "kernel/vfs/vfs.h"

/**
 * Mounts /dev directory.
 */
vfs_node_t* device_mount();

/**
 * Unmounts /dev directory and clears resources.
 */
void device_unmount();

/**
 * Mounts TTY file to /dev/tty. Called by device_mount().
 */
vfs_node_t* device_tty_mount();

/**
 * Unmounts /dev/tty and clears resources.  Called by device_unmount().
 */
void device_tty_unmount();

#endif
