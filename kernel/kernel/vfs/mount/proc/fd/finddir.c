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

    errno = 0;
    // Attempt to read FD ID from name
    uint32_t fd_id = strtol(name, NULL, 10);
    if (errno != 0) {
        return NULL;
    }

    task_file_descriptor_t* fd;
    if (!proc_find_fd(meta->pid, fd_id, &fd)) {
        return NULL;
    }

    vfs_node_t* result = kmalloc(sizeof(vfs_node_t));
    vfs_populate_node(result, name, VFS_TYPE_FILE);
    result->on_release = proc_on_release;

    proc_inode_meta_fd_t* new_meta = kmalloc(sizeof(proc_inode_meta_fd_t));
    new_meta->pid = meta->pid;
    new_meta->fd_id = fd_id;
    result->metadata = new_meta;
    result->open_node = proc_fd_open;
    result->close_node = proc_fd_close;
    result->read_node = proc_fd_read;
    result->write_node = proc_fd_write;
    result->ioctl_node = proc_fd_ioctl;
    result->stat_node = proc_fd_stat;
    return result;
}
