#include "kernel/vfs/pipe.h"
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
    return -ENOTSUP;
}
