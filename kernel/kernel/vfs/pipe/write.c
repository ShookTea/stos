#include "./pipe.h"
#include "errno.h"
#include "libds/libds.h"
#include "libds/ringbuf.h"
#include <stdio.h>

size_t pipe_write(
    vfs_file_t* file,
    size_t off __attribute__((unused)),
    size_t size,
    const void* ptr
) {
    if (file == NULL || ptr == NULL || file->metadata == NULL) {
        return 0;
    }

    pipe_file_meta_t* file_meta = file->metadata;

    if (file->dentry == NULL || file->dentry->inode == NULL) {
        return 0;
    }
    if (file->dentry->inode->metadata == NULL) {
        return 0;
    }
    pipe_node_meta_t* node_meta = file->dentry->inode->metadata;
    ds_ringbuf_t* ringbuf = node_meta->ringbuf;

    if (ringbuf == NULL) {
        return 0;
    }

    if (!node_meta->write_opened || node_meta->write_closed) {
        return 0;
    }
    if (file_meta->read_side || !file->writeable) {
        return 0;
    }

    if (!node_meta->read_opened || node_meta->read_closed) {
        // If read side is not open, writing should report EPIPE
        return -EPIPE;
    }

    size_t empty_space = ds_ringbuf_free_space(ringbuf);

    size_t to_write = empty_space < size ? empty_space : size;
    size_t written = 0;
    for (size_t i = 0; i < to_write; i++) {
        if (ds_ringbuf_push(ringbuf, ptr + i) != DS_SUCCESS) {
            break;
        }
        written++;
    }

    return written;
}
