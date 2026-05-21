#include <stdio.h>
#include <string.h>
#include "../proc.h"
#include "kernel/task/task.h"
#include "kernel/vfs/vfs.h"
#include "kernel/memory/kmalloc.h"

vfs_node_t* proc_pid_finddir(
    vfs_node_t* node,
    char* name
) {
    proc_inode_meta_pid_t* meta = node->metadata;
    task_t* task = task_get_task_by_pid(meta->pid);
    if (task == NULL) {
        return NULL;
    }

    if (!strcmp(name, "stat")) {
        vfs_node_t* result = kmalloc(sizeof(vfs_node_t));
        vfs_populate_node(result, "stat", VFS_TYPE_FILE);
        result->on_release = proc_on_release;
        proc_inode_meta_pid_t* res_meta = kmalloc(
            sizeof(proc_inode_meta_pid_t)
        );
        res_meta->pid = meta->pid;
        result->metadata = res_meta;
        result->read_node = proc_pid_read_stat;
        return result;
    }

    return NULL;
}
