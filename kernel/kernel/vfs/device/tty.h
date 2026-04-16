#ifndef INCLUDE_KERNEL_SRC_VFS_DEVICE_TTY_H
#define INCLUDE_KERNEL_SRC_VFS_DEVICE_TTY_H

#include "kernel/task/wait.h"

#define TTY_BUFFER_SIZE 4096

typedef struct {
    wait_obj_t* wait_obj;
} tty_state_t;

#endif
