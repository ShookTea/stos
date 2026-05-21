#include "../proc.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/task/task.h"

void proc_fd_open(vfs_node_t* node, vfs_file_t* file, uint8_t mode)
{
    proc_inode_meta_fd_t* meta = node->metadata;
    task_file_descriptor_t* fd;
    if (!proc_find_fd(meta->pid, meta->fd_id, &fd)) {
        return;
    }

    vfs_file_t* real = vfs_open(fd->file->dentry, mode);

    proc_file_meta_fd_t* new_meta = kmalloc(sizeof(proc_file_meta_fd_t));
    new_meta->fd_id = meta->fd_id;
    new_meta->pid = meta->pid;
    new_meta->opened_file = real;

    file->metadata = new_meta;
}
