#include <stdio.h>
#include <string.h>
#include "../proc.h"
#include "kernel/task/scheduler.h"
#include "kernel/task/task.h"

// The amount of nodes in /proc that should only exist when running from
// a process
#define TARGET_COUNT_FOR_CURR_TASK 1

bool proc_root_readdir(
    vfs_node_t* node __attribute__((unused)),
    size_t index,
    struct dirent* out
) {
    // Iterate over proccesses by ID
    size_t task_count = task_get_tasks_count();
    if (index < task_count) {
        task_t* task = task_get_task_by_index(index);
        out->ino = task->pid;
        sprintf(out->name, "%u", task->pid);
        return true;
    }

    task_t* curr = scheduler_get_current_task();
    if (curr == NULL) {
        // Skip immediately to entries that are only valid for current task
        index += TARGET_COUNT_FOR_CURR_TASK;
    }

    // Add "self" symlink
    if (index == task_count) {
        out->ino = curr->pid;
        strcpy(out->name, "self");
        return true;
    }

    return false;
}
