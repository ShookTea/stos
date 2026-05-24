#include <stdio.h>
#include <string.h>
#include <dirent.h>
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

    out->d_ino = index;

    if (index == 0) {
        strcpy(out->d_name, "stat");
        out->d_type = DT_REG;
        return true;
    }
    if (index == 1) {
        strcpy(out->d_name, "fd");
        out->d_type = DT_DIR;
        return true;
    }

    return false;
}
