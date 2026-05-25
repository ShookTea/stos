#include "kernel/vfs/pipe.h"
#include "./pipe.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/vfs/vfs.h"
#include "libds/ringbuf.h"
#include <errno.h>
#include <stddef.h>
#include "kernel/debug.h"

#define _debug_puts(...) debug_puts_c(DBC_VFS_PIPE, __VA_ARGS__)
#define _debug_printf(...) debug_printf_c(DBC_VFS_PIPE, __VA_ARGS__)

static ino_t ino_iterator = 0;

static void pipe_on_release(
    vfs_node_t* node
) {

    if (node == NULL || node->metadata == NULL) return;
    pipe_node_meta_t* meta = node->metadata;
    if (meta->ringbuf != NULL) {
        ds_ringbuf_destroy(meta->ringbuf);
        meta->ringbuf = NULL;
    }

    kfree(meta);
}

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

    // Create node
    vfs_node_t* node = kmalloc_flags(sizeof(vfs_node_t), KMALLOC_ZERO);
    node->inode = ino_iterator++;
    vfs_populate_node(node, "pipe", VFS_TYPE_FIFO);
    node->on_release = pipe_on_release;
    node->open_node = pipe_open;
    node->close_node = pipe_close;
    node->read_node = pipe_read;
    node->write_node = pipe_write;
    pipe_node_meta_t* meta = kmalloc_flags(
        sizeof(pipe_node_meta_t),
        KMALLOC_ZERO
    );
    node->metadata = meta;
    meta->ringbuf = ds_ringbuf_create(PIPE_BUF_SIZE, 1, true);
    meta->read_closed = false;
    meta->write_closed = false;
    meta->read_opened = false;
    meta->write_opened = false;

    _debug_printf(
        "Created pipe #%u for task %u '%s'\n",
        node->inode,
        task->pid,
        task->name
    );

    return -ENOTSUP;
}
