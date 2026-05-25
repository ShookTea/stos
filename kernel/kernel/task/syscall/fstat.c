#include <stdbool.h>
#include <errno.h>
#include "kernel/vfs/vfs.h"
#include "kernel/task/task.h"
#include "kernel/task/syscall.h"

int sys_fstat(int fd_id, struct stat* stat)
{
    task_file_descriptor_t* fd = syscall_get_descriptor_by_fd(fd_id);

    if (fd == NULL) {
        return -ENOENT;
    }

    return -vfs_stat(fd->file->dentry, stat);
}
