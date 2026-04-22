#include "kernel/vfs/vfs.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/debug.h"
#include <stdint.h>

#define _debug_puts(...) debug_puts_c("VFS/mount/ext2", __VA_ARGS__)
#define _debug_printf(...) debug_printf_c("VFS/mount/ext2", __VA_ARGS__)

vfs_mount_result_t vfs_mount_ext2(
    dentry_t* device_file,
    dentry_t* target,
    uint16_t flags __attribute__((unused)),
    const void* data __attribute__((unused))
) {
    char* device_path = kmalloc_flags(VFS_MAX_PATH_LENGTH, KMALLOC_ZERO);
    char* target_path = kmalloc_flags(VFS_MAX_PATH_LENGTH, KMALLOC_ZERO);
    vfs_build_absolute_path(
        vfs_get_real_root(),
        device_file,
        device_path,
        VFS_MAX_PATH_LENGTH
    );
    vfs_build_absolute_path(
        vfs_get_real_root(),
        target,
        target_path,
        VFS_MAX_PATH_LENGTH
    );
    _debug_printf(
        "Attempting to mount device %s at target %s\n",
        device_path,
        target_path
    );
    kfree(device_path);
    kfree(target_path);

    return MOUNT_ERR_DEVICE_NOT_IN_FORMAT;
}
