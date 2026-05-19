#include <stddef.h>
#include <errno.h>
#include "kernel/task/syscall.h"
#include "kernel/task/task.h"
#include "kernel/vfs/vfs.h"
#include <kernel/memory/vmm.h>
#include <kernel/terminal.h>

int sys_lseek(int fd_id, int offset, int whence)
{
    // Get file descriptor
    task_file_descriptor_t* fd = syscall_get_descriptor_by_fd(fd_id);
    if (fd == NULL) {
        // TODO: set error
        return -EBADF;
    }

    int new_offset = 0;
    switch (whence) {
        case SYS_LSEEK_OPT_SEEK_SET:
            new_offset = offset;
            break;
        case SYS_LSEEK_OPT_SEEK_CUR:
            new_offset = fd->file->offset + offset;
            break;
        case SYS_LSEEK_OPT_SEEK_END:
            new_offset = fd->file->dentry->inode->length + offset;
            break;
        default:
            return -EINVAL;
    }

    vfs_seek(fd->file, new_offset);

    return fd->file->offset;
}
