#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "../proc.h"
#include "kernel/task/task.h"
#include "kernel/vfs/vfs.h"
#include "kernel/memory/kmalloc.h"

vfs_node_t* proc_fd_finddir(
    vfs_node_t* node,
    char* name
) {
    proc_inode_meta_pid_t* meta = node->metadata;
    task_t* task = task_get_task_by_pid(meta->pid);
    if (task == NULL) {
        return NULL;
    }

    errno = 0;
    // Attempt to read FD ID from name
    uint32_t fd_id = strtol(name, NULL, 10);
    if (errno != 0) {
        return NULL;
    }

    if (fd_id >= task->fd_count) {
        return NULL;
    }

    task_file_descriptor_t* fd = task->fd[fd_id];
    if (fd->file == NULL) {
        return NULL;
    }

    vfs_node_t* result = kmalloc(sizeof(vfs_node_t));
    vfs_populate_node(result, name, VFS_TYPE_FILE);
    result->on_release = proc_on_release;
    return result;
}
