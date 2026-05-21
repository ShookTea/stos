#include "proc.h"
#include "../mount.h"
#include "kernel/task/task.h"
#include "kernel/vfs/vfs.h"
#include "kernel/memory/kmalloc.h"

void proc_on_release(vfs_node_t* node)
{
    if (node == NULL) {
        return;
    }

    if (node->metadata != NULL) {
        kfree(node->metadata);
    }
    kfree(node);
}

vfs_mount_result_t vfs_mount_proc(
    dentry_t* target
) {
    target->inode->readdir_node = proc_root_readdir;
    target->inode->finddir_node = proc_root_finddir;
    return MOUNT_SUCCESS;
}

bool proc_find_fd(uint32_t pid, size_t fd_id, task_file_descriptor_t** dest)
{
    if (dest == NULL) {
        return false;
    }
    task_t* task = task_get_task_by_pid(pid);
    if (task == NULL) {
        return false;
    }
    if (fd_id >= task->fd_count) {
        return false;
    }

    task_file_descriptor_t* fd = task->fd[fd_id];
    if (fd->file == NULL) {
        return false;
    }

    *dest = fd;
    return true;
}
