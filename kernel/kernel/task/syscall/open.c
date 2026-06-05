#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include "kernel/task/syscall.h"
#include "kernel/task/scheduler.h"
#include "kernel/task/task.h"
#include "kernel/vfs/vfs.h"
#include "kernel/vfs/path.h"

uint32_t sys_open(const char* path, uint32_t flags)
{
    task_t* current = scheduler_get_current_task();
    if (current == NULL) {
        return -ENOTSUP;
    }

    char* basename = NULL;
    bool file_exists = false;
    dentry_t* node = path_resolve_optional(
        current->root_node,
        current->working_directory,
        path,
        &basename,
        &file_exists
    );
    if (node == NULL) {
        // Neither file nor its parent was found
        return -ENOENT;
    }
    if (!file_exists && !(flags & O_CREAT)) {
        // File doesn't exist, and user didn't requested to create one
        return -ENOENT;
    }
    if (!file_exists && (flags & O_CREAT)) {
        // File doesn't exist, and user request to create one
        dentry_t* parent = node;
        int mkfile_res = vfs_mkfile(parent, basename);
        if (mkfile_res != 0) {
            vfs_dentry_unref(parent);
            return -mkfile_res;
        }

        node = vfs_finddir(parent, basename);
        if (node == NULL) {
            return -ENOENT;
        }
    }

    if (node == NULL) {
        return -ENOENT;
    }

    int err_from_open = 0;
    vfs_file_t* handler = vfs_open(node, flags, &err_from_open);
    if (handler == NULL) {
        return err_from_open == 0 ? -EIO : -err_from_open;
    }

    task_file_descriptor_t* desc = task_add_fd(current, handler);
    if (desc == NULL) {
        return -EIO;
    }
    return desc->identifier;
}
