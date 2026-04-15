#ifndef INCLUDE_KERNEL_SRC_VFS_MOUNT_ISO9660_H
#define INCLUDE_KERNEL_SRC_VFS_MOUNT_ISO9660_H

#include "kernel/vfs/vfs.h"
#include "kernel/task/wait.h"

#define ISO_SECTOR_SIZE 2048

typedef struct {
    dentry_t* device_file;
    dentry_t* target;
    uint8_t* buffer;
    wait_obj_t* wait_obj;
    bool completed;
    vfs_mount_result_t result;
} mount_task_t;

#endif
