#include <dirent.h>
#include <errno.h>
#include <stddef.h>
#include "kernel/task/syscall.h"

int sys_getdents(int fd_id, struct dirent* dir, int count)
{
    // Get file descriptor
    task_file_descriptor_t* fd = syscall_get_descriptor_by_fd(fd_id);
    if (fd == NULL) return -EBADF;

    vfs_file_t* file = fd->file;
    if (file == NULL) return -EBADF;

    dentry_t* dentry = fd->file->dentry;
    if (dentry == NULL || dentry->inode == NULL) return -EBADF;

    if (dentry->inode->type != VFS_TYPE_DIRECTORY)
    {
        return -ENOTDIR;
    }

    int index = 0;
    bool reached_end = false;
    while (index < count && !reached_end) {
        bool res = vfs_readdir(dentry, file->offset, dir + index);
        if (res) {
            index++;
            file->offset++;
        } else {
            reached_end = true;
        }
    }

    return index;
}
