#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "../proc.h"
#include "kernel/task/scheduler.h"
#include "kernel/task/task.h"
#include "kernel/vfs/vfs.h"
#include "kernel/memory/kmalloc.h"

static int proc_self_readlink(
    const vfs_node_t* node __attribute__((unused)),
    char* buf,
    size_t len
) {
    task_t* task = scheduler_get_current_task();
    if (task == NULL) {
        return -ENOTSUP;
    }
    return snprintf(buf, len, "./%u", task->pid);
}

vfs_node_t* proc_root_finddir(
    vfs_node_t* node __attribute__((unused)),
    char* name
) {
    task_t* task = scheduler_get_current_task();
    if (!strcmp(name, "self")) {
        if (task == NULL) {
            return NULL;
        }
        vfs_node_t* result = kmalloc(sizeof(vfs_node_t));
        vfs_populate_node(result, "self", VFS_TYPE_SYMLINK);
        result->readlink_node = proc_self_readlink;
        return result;
    }

    errno = 0;
    // Attempt to read process PID from name
    uint32_t pid __attribute__((unused)) = strtol(name, NULL, 10);
    if (errno == 0) {
        vfs_node_t* result = kmalloc(sizeof(vfs_node_t));
        vfs_populate_node(result, name, VFS_TYPE_DIRECTORY);
        return result;
    }

    return NULL;
}
