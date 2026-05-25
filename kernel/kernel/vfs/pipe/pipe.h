#ifndef INCLUDE_KERNEL_SRC_VFS_PIPE_H
#define INCLUDE_KERNEL_SRC_VFS_PIPE_H

#include <libds/ringbuf.h>

// buffer size in bytes
#define PIPE_BUF_SIZE 4096

typedef struct {
    ds_ringbuf_t* ringbuf;
    bool read_closed;
    bool write_closed;
} pipe_node_meta_t;

#endif
