#ifndef INCLUDE_KERNEL_SRC_VFS_PIPE_H
#define INCLUDE_KERNEL_SRC_VFS_PIPE_H

#include "kernel/task/wait.h"
#include "kernel/vfs/vfs.h"
#include <libds/ringbuf.h>
#include <stddef.h>

// buffer size in bytes
#define PIPE_BUF_SIZE 4096

typedef struct {
    ds_ringbuf_t* ringbuf;
    uint32_t read_ref_count;
    uint32_t write_ref_count;
    wait_obj_t* wait_obj;
} pipe_node_meta_t;

typedef struct {
    bool read_side;
} pipe_file_meta_t;

int pipe_open(vfs_node_t* node, vfs_file_t* file, uint8_t mode);
void pipe_close(vfs_node_t* node, vfs_file_t* file);
size_t pipe_read(vfs_file_t* file, size_t off, size_t size, void* ptr);
size_t pipe_write(vfs_file_t* file, size_t off, size_t size, const void* ptr);

#endif
