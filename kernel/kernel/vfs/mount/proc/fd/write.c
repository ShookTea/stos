#include "../proc.h"
#include "kernel/vfs/vfs.h"

size_t proc_fd_write(vfs_file_t* file, size_t off, size_t size, const void* ptr)
{
    proc_file_meta_fd_t* meta = file->metadata;
    if (meta == NULL) {
        return 0;
    }

    task_file_descriptor_t* fd;
    if (!proc_find_fd(meta->pid, meta->fd_id, &fd)) {
        return 0;
    }

    vfs_file_t* real = meta->opened_file;
    if (real != NULL) {
        return 0;
    }

    vfs_seek(real, off);
    return vfs_write(real, size, ptr);
}
