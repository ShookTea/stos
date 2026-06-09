#include "errno.h"
#include "kernel/task/scheduler.h"
#include "kernel/task/syscall.h"
#include "kernel/task/task.h"
#include "kernel/vfs/path.h"
#include "kernel/vfs/vfs.h"
#include <fcntl.h>
#include <stdbool.h>

int sys_mkdir(
    int dirfd,
    const char* path,
    mode_t mode __attribute__((unused)) // TODO: add support for mode
)
{
    task_t* task = scheduler_get_current_task();
    if (task == NULL) {
        return -ENOTSUP;
    }

    dentry_t* relative_base;
    if (dirfd == AT_FDCWD) {
        relative_base = task->working_directory;
    } else {
        task_file_descriptor_t* fd = syscall_get_descriptor_by_fd(dirfd);
        if (fd == NULL || fd->file == NULL) {
            return -EBADF;
        }
        relative_base = fd->file->dentry;
    }

    if (relative_base == NULL) {
        return -EBADF;
    }
    if (!(relative_base->inode->type & VFS_TYPE_DIRECTORY)) {
        return -ENOTDIR;
    }

    char* basename = NULL;
    bool file_already_exists = false;
    dentry_t* parent_node = path_resolve_optional(
        task->root_node,
        relative_base,
        path,
        &basename,
        &file_already_exists
    );

    if (parent_node == NULL) {
        // Neither file nor its parent was found
        return -ENOENT;
    }

    if (file_already_exists) {
        return -EEXIST;
    }

    int res = vfs_mkdir(parent_node, basename);
    if (res != 0) {
        return -res;
    }

    return 0;
}
