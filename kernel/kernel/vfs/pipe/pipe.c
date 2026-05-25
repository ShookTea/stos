#include "kernel/vfs/pipe.h"
#include "./pipe.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/vfs/vfs.h"
#include <errno.h>
#include <stddef.h>

int pipe_create(task_t* task, int* read_fd, int* write_fd, int flags)
{
    if (task == NULL) {
        return -ENOTSUP;
    }
    if (read_fd == NULL || write_fd == NULL) {
        return -EFAULT;
    }
    if (flags != 0) {
        return -EINVAL;
    }

    vfs_node_t* node = kmalloc_flags(sizeof(vfs_node_t), KMALLOC_ZERO);
    vfs_populate_node(node, "pipe", VFS_TYPE_FIFO);
    pipe_node_meta_t* meta = kmalloc_flags(
        sizeof(pipe_node_meta_t),
        KMALLOC_ZERO
    );
    node->metadata = meta;
    meta->ringbuf = ds_ringbuf_create(PIPE_BUF_SIZE, 1, true);
    meta->read_closed = false;
    meta->write_closed = false;

    return -ENOTSUP;
}
