#include "../proc.h"
#include "kernel/vfs/vfs.h"

size_t proc_fd_write(vfs_file_t* file, size_t off, size_t size, const void* ptr)
{
    vfs_file_t* real = file->metadata;
    vfs_seek(real, off);
    return vfs_write(real, size, ptr);
}
