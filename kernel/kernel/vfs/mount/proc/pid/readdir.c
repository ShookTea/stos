#include <stdio.h>
#include <string.h>
#include "../proc.h"
#include "kernel/task/scheduler.h"
#include "kernel/task/task.h"

bool proc_pid_readdir(
    vfs_node_t* node __attribute__((unused)),
    size_t index,
    struct dirent* out
) {
    task_t* curr = scheduler_get_current_task();
    if (curr == NULL) {
        return NULL;
    }

    out->ino = 0;

    if (index == 0) {
        strcpy(out->name, "stat");
        return true;
    }

    return false;
}
