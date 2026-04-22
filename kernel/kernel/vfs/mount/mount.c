#include "kernel/vfs/vfs.h"
#include "./mount.h"
#include <stdint.h>
#include <string.h>

vfs_mount_result_t vfs_mount(
    dentry_t* device_file,
    dentry_t* target,
    const char* filesystem,
    uint16_t flags,
    const void* data
) {
    if (device_file == NULL || target == NULL || filesystem == NULL) {
        return MOUNT_ERR_NULL_POINTER;
    }

    if (!(target->inode->type & VFS_TYPE_DIRECTORY)) {
        return MOUNT_ERR_TARGET_NOT_DIR;
    }

    if (!strcmp(filesystem, MOUNT_FILESYSTEM_ISO9660)) {
        return vfs_mount_iso9660(device_file, target, flags, data);
    }
    if (!strcmp(filesystem, MOUNT_FILESYSTEM_EXT2)) {
        return vfs_mount_ext2(device_file, target, flags, data);
    }
    return MOUNT_ERR_UNKNOWN_FILESYSTEM;
}
