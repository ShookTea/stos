#include <stdio.h>
#include <string.h>
#include "../proc.h"
#include "kernel/task/task.h"

bool proc_pid_readdir(
    vfs_node_t* node,
    size_t index,
    struct dirent* out
) {
    proc_inode_meta_pid_t* meta = node->metadata;
    task_t* task = task_get_task_by_pid(meta->pid);
    if (task == NULL) {
        return false;
    }

    out->ino = index;

    if (index == 0) {
        strcpy(out->name, "stat");
        return true;
    }
    if (index == 1) {
        strcpy(out->name, "fd");
        return true;
    }

    return false;
}
