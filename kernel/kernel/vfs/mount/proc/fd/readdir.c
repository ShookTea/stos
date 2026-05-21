#include <stdio.h>
#include <string.h>
#include "../proc.h"
#include "kernel/task/task.h"

bool proc_fd_readdir(
    vfs_node_t* node,
    size_t index,
    struct dirent* out
) {
    proc_inode_meta_pid_t* meta = node->metadata;
    task_t* task = task_get_task_by_pid(meta->pid);
    if (task == NULL) {
        return false;
    }

    if (index >= task->fd_count) {
        // It's certainly outside of FD count, no matter if they're open or not
        return false;
    }

    // We want to iterate over _open_ FDs and return the one at position "index"
    size_t opened_index = 0;
    for (size_t i = 0; i < task->fd_count; i++) {
        task_file_descriptor_t* fd = task->fd[i];
        if (fd->file == NULL) {
            // Not open
            continue;
        }

        // FD is open.
        if (opened_index == index) {
            out->ino = i;
            sprintf(out->name, "%u", i);
            return true;
        } else {
            opened_index++;
        }
    }

    return false;
}
