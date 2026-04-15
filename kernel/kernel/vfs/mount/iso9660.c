#include "kernel/vfs/vfs.h"
#include "./mount.h"
#include <stdint.h>

vfs_mount_result_t vfs_mount_iso9660(
    dentry_t* device_file,
    dentry_t* target __attribute__((unused)),
    uint16_t flags __attribute__((unused)),
    const void* data __attribute__((unused))
) {
    // First check: all ISO9660 files must have a size of at least 0x11 * 2 KiB:
    // - sector size in ISO9660 is 2 KiB
    // - sector 0x10 contains first volume descriptor
    if (device_file->inode->length < (0x11 * 2048)) {
        return MOUNT_ERR_DEVICE_NOT_IN_FORMAT;
    }

    return MOUNT_ERR_UNKNOWN_FILESYSTEM;
}
