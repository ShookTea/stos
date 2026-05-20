#include "proc.h"
#include "../mount.h"
#include "kernel/vfs/vfs.h"

vfs_mount_result_t vfs_mount_proc(
    dentry_t* target
) {
    target->inode->readdir_node = proc_root_readdir;
    target->inode->finddir_node = proc_root_finddir;
    return MOUNT_SUCCESS;
}
