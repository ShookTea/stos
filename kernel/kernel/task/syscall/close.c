#include <stdint.h>
#include <errno.h>
#include "../syscall.h"
#include "kernel/task/task.h"
#include "kernel/vfs/vfs.h"

int sys_close(int fd)
{
    task_file_descriptor_t* desc = syscall_get_descriptor_by_fd(fd);
    if (desc == NULL) {
        return -ENOTSUP;
    }

    vfs_close(desc->file);
    desc->file = NULL;

    return 0;
}
