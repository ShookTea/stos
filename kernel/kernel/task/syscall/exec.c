#include "../syscall.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/task/task.h"
#include "kernel/vfs/vfs.h"
#include <stdint.h>

int sys_exec(const char* path)
{
    vfs_node_t* node = vfs_resolve(path);
    if (node == NULL) {
        return SYSCALL_ERROR;
    }
    if (!(node->type & VFS_TYPE_FILE)) {
        return SYSCALL_ERROR;
    }

    vfs_file_t* file = vfs_open(node, VFS_MODE_READONLY);
    if (file == NULL) {
        return SYSCALL_ERROR;
    }

    size_t size = file->node->length;
    void* buf = kmalloc(size);
    if (buf == NULL) {
        vfs_close(file);
        return SYSCALL_ERROR;
    }

    vfs_read(file, size, buf);
    vfs_close(file);

    int ret = task_exec(buf, size);
    kfree(buf);
    return ret;
}
