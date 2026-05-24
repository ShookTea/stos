#ifndef INCLUDE_KERNEL_SRC_VFS_MOUNT_PROC_H
#define INCLUDE_KERNEL_SRC_VFS_MOUNT_PROC_H

#include "kernel/task/task.h"
#include "kernel/vfs/vfs.h"

typedef struct {
    uint32_t pid;
} proc_inode_meta_pid_t;

typedef struct {
    uint32_t pid;
    uint32_t fd_id;
} proc_inode_meta_fd_t;

typedef struct {
    uint32_t pid;
    uint32_t fd_id;
    vfs_file_t* opened_file;
} proc_file_meta_fd_t;

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

// Functions for reading files in /proc/<pid> directories
size_t proc_pid_read_stat(
    vfs_file_t* file,
    size_t offset,
    size_t size,
    void* ptr
);

// Functions for /proc/<pid>/fd directories (using proc_inode_meta_pid_t)
bool proc_fd_readdir(vfs_node_t* node, size_t index, struct dirent* out);
vfs_node_t* proc_fd_finddir(vfs_node_t* node, char* name);

// Proxy functions for /proc/<pid>/fd/<fd_id> files (using proc_inode_meta_fd_t)
int proc_fd_open(vfs_node_t* node, vfs_file_t* file, uint8_t mode);
void proc_fd_close(vfs_node_t* node, vfs_file_t* file);
size_t proc_fd_read(vfs_file_t* file, size_t offset, size_t size, void* ptr);
size_t proc_fd_write(vfs_file_t* file, size_t off, size_t size, const void* ptr);
int proc_fd_ioctl(vfs_file_t* file, uint32_t op, void* arg);
int proc_fd_stat(const vfs_node_t* node, struct stat* stat);

/**
 * Find file descriptor `fd_id` within process `pid`. If found, store it in
 * `dest` and return `true`, otherwise return `false`.
 */
bool proc_find_fd(uint32_t pid, size_t fd_id, task_file_descriptor_t** dest);

#endif
