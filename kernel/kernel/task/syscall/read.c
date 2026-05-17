#include <stddef.h>
#include <errno.h>
#include "../syscall.h"
#include "kernel/task/task.h"
#include "kernel/vfs/vfs.h"
#include <kernel/memory/vmm.h>
#include <kernel/terminal.h>

int sys_read(int fd_id, void* buf, size_t count)
{
    // Get file descriptor
    task_file_descriptor_t* fd = syscall_get_descriptor_by_fd(fd_id);
    if (fd == NULL) {
        return -EBADF;
    }

    size_t result = vfs_read(fd->file, count, buf);
    return result;
}
