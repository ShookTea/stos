#ifndef INCLUDE_KERNEL_SRC_VFS_DEVICE_TTY_H
#define INCLUDE_KERNEL_SRC_VFS_DEVICE_TTY_H

#include "kernel/task/wait.h"
#include <libds/ringbuf.h>

#define TTY_BUFFER_SIZE 4096

typedef struct {
    // Queue of processes waiting for read from the TTY
    wait_obj_t* wait_obj;
    // Buffer of all "commited" bytes that are ready to read
    ds_ringbuf_t* buffer;
    size_t ready_lines;

} tty_state_t;

#endif
