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

#endif
