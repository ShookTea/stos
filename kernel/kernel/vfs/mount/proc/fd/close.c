#include "../proc.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/vfs/vfs.h"

void proc_fd_close(vfs_node_t* node __attribute__((unused)), vfs_file_t* file)
{
    proc_file_meta_fd_t* meta = file->metadata;
    if (meta == NULL) {
        return;
    }

    task_file_descriptor_t* fd;
    if (!proc_find_fd(meta->pid, meta->fd_id, &fd)) {
        return;
    }

    if (meta->opened_file != NULL) {
        vfs_close(meta->opened_file);
    }
    kfree(file->metadata);
}
