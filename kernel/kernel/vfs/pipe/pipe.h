#ifndef INCLUDE_KERNEL_SRC_VFS_PIPE_H
#define INCLUDE_KERNEL_SRC_VFS_PIPE_H

#include "kernel/vfs/vfs.h"
#include <libds/ringbuf.h>

// buffer size in bytes
#define PIPE_BUF_SIZE 4096

typedef struct {
    ds_ringbuf_t* ringbuf;
    bool read_opened;
    bool write_opened;
    bool read_closed;
    bool write_closed;
} pipe_node_meta_t;

typedef struct {
    bool read_side;
} pipe_file_meta_t;

int pipe_open(vfs_node_t* node, vfs_file_t* file, uint8_t mode);
void pipe_close(vfs_node_t* node, vfs_file_t* file);

#endif
