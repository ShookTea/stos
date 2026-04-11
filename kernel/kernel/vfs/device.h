#ifndef INCLUDE_KERNEL_SRC_VFS_DEVICE_H
#define INCLUDE_KERNEL_SRC_VFS_DEVICE_H

#include "kernel/vfs/vfs.h"

/**
 * Mounts TTY file to /dev/tty. Called by device_mount().
 */
vfs_node_t* device_tty_mount();

/**
 * Unmounts /dev/tty and clears resources.  Called by device_unmount().
 */
void device_tty_unmount();

/**
 * Mounts hard drives to /dev/hda, /dev/hdb etc.. Called by device_mount().
 * Returns NULL-terminated list of nodes.
 */
vfs_node_t** device_hd_mount();

/**
 * Unmounts hard drives and clears resources. Called by device_unmount().
 */
void device_hd_unmount();

#endif
