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
 * Mounts hard disk-level nodes (/dev/hda, /dev/hdb, /dev/sr0, …).
 * Partition nodes are NOT created here — they are generated on demand.
 * Returns a NULL-terminated list of nodes.
 */
vfs_node_t** device_hd_mount();

/**
 * Unmounts hard drives and clears resources. Called by device_unmount().
 */
void device_hd_unmount();

/**
 * Returns the partition dirent at part_index across all PIO disks.
 * When part_index == 0 the partition cache is refreshed via MBR reads.
 * Returns false when there is no partition at that index.
 */
bool device_hd_readdir_partition(size_t part_index, struct dirent* out);

/**
 * Returns a dynamically-allocated vfs_node_t for the named partition
 * (e.g. "hda1"), reading the current MBR to determine its location.
 * The node has on_release set and will free itself when open_count drops to 0.
 * Returns NULL if the partition does not exist.
 */
vfs_node_t* device_hd_finddir_partition(const char* name);

#endif
