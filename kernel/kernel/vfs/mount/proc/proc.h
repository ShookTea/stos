#ifndef INCLUDE_KERNEL_SRC_VFS_MOUNT_PROC_H
#define INCLUDE_KERNEL_SRC_VFS_MOUNT_PROC_H

#include "kernel/vfs/vfs.h"

typedef struct {
    uint32_t pid;
} proc_inode_meta_pid_t;

/**
 * General-purpose entry that will deallocate node and meta (if present)
 */
void proc_on_release(vfs_node_t* node);

// Functions for root directory
bool proc_root_readdir(vfs_node_t* node, size_t index, struct dirent* out);
vfs_node_t* proc_root_finddir(vfs_node_t* node, char* name);

// Functions for /proc/<pid> directories (using proc_inode_meta_pid_t)
bool proc_pid_readdir(vfs_node_t* node, size_t index, struct dirent* out);
vfs_node_t* proc_pid_finddir(vfs_node_t* node, char* name);

#endif
