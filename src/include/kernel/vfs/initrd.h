#ifndef INCLUDE_KERNEL_VFS_INITRD_H
#define INCLUDE_KERNEL_VFS_INITRD_H

#include <kernel/vfs/vfs.h>

/**
 * Attempts to load initrd (passed as .tar file) from memory and returns a valid
 * vfs_node_t, or NULL if initrd was not present. Will cause kernel panic if
 * initrd is present, but invalid.
 */
vfs_node_t* initrd_mount();
#endif
