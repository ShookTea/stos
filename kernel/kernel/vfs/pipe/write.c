#include "./pipe.h"
#include "errno.h"
#include "kernel/task/wait.h"
#include "kernel/vfs/vfs.h"
#include "libds/libds.h"
#include "libds/ringbuf.h"
#include <stdio.h>
#include "kernel/debug.h"

#define _debug_puts(...) debug_puts_cc(DBC_VFS_PIPE, "write", __VA_ARGS__)
#define _debug_printf(...) debug_printf_cc(DBC_VFS_PIPE, "write", __VA_ARGS__)

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
    vfs_node_t* node = file->dentry->inode;
    pipe_node_meta_t* node_meta = node->metadata;
    ds_ringbuf_t* ringbuf = node_meta->ringbuf;

    if (ringbuf == NULL) {
        return 0;
    }

    if (node_meta->write_ref_count == 0) {
        return 0;
    }
    if (file_meta->read_side || !file->writeable) {
        return 0;
    }

    if (node_meta->read_ref_count == 0) {
        _debug_printf("Read side of pipe #%u is not open\n", node->inode);
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

    if (written > 0) {
        wait_wake_up(node_meta->wait_obj);
    }

    return written;
}
