#ifndef INCLUDE_KERNEL_SRC_VFS_MOUNT_H
#define INCLUDE_KERNEL_SRC_VFS_MOUNT_H

#include "kernel/vfs/vfs.h"
#include <stdint.h>

vfs_mount_result_t vfs_mount_iso9660(
    dentry_t* device_file,
    dentry_t* target,
    uint16_t flags,
    const void* data
);

vfs_mount_result_t vfs_mount_ext2(
    dentry_t* device_file,
    dentry_t* target,
    uint16_t flags,
    const void* data
);

#endif
