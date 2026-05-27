#include "./pipe.h"
#include "kernel/task/wait.h"
#include "libds/libds.h"
#include "libds/ringbuf.h"
#include <stdio.h>
#include "kernel/debug.h"

#define _debug_puts(...) debug_puts_cc(DBC_VFS_PIPE, "read", __VA_ARGS__)
#define _debug_printf(...) debug_printf_cc(DBC_VFS_PIPE, "read", __VA_ARGS__)

static bool is_buffer_ready(void* arg)
{
    pipe_node_meta_t* meta = arg;
    if (meta->write_ref_count == 0) {
        // don't block if all write refs are closed
        return true;
    }
    return ds_ringbuf_size(meta->ringbuf) > 0;
}

size_t pipe_read(
    vfs_file_t* file,
    size_t off __attribute__((unused)),
    size_t size,
    void* ptr
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
    vfs_node_t* node = file->dentry->inode;
    pipe_node_meta_t* node_meta = node->metadata;
    ds_ringbuf_t* ringbuf = node_meta->ringbuf;

    if (ringbuf == NULL) {
        return 0;
    }

    if (node_meta->read_ref_count == 0) {
        return 0;
    }
    if (!file_meta->read_side || !file->readable) {
        return 0;
    }

    wait_on_condition(node_meta->wait_obj, is_buffer_ready, node_meta);

    size_t curr_size = ds_ringbuf_size(ringbuf);

    if (curr_size == 0 && node_meta->write_ref_count == 0) {
        _debug_printf("Write side of pipe #%u is not open\n", node->inode);
        return 0;
    }

    size_t to_read = curr_size < size ? curr_size : size;
    size_t read = 0;
    for (size_t i = 0; i < to_read; i++) {
        if (ds_ringbuf_pop(ringbuf, ptr + i) != DS_SUCCESS) {
            break;
        }
        read++;
    }

    return read;
}
