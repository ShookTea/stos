#ifndef INCLUDE_KERNEL_SRC_VFS_MOUNT_PROC_H
#define INCLUDE_KERNEL_SRC_VFS_MOUNT_PROC_H

#include "kernel/vfs/vfs.h"

// Functions for root directory
bool proc_root_readdir(vfs_node_t* node, size_t index, struct dirent* out);
vfs_node_t* proc_root_finddir(vfs_node_t* node, char* name);

#endif
