#include "../mount.h"
#include "kernel/vfs/vfs.h"

vfs_mount_result_t vfs_mount_proc(
    dentry_t* target __attribute__((unused))
) {
    return MOUNT_SUCCESS;
}
