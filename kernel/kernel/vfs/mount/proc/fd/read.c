#include "../proc.h"
#include "kernel/vfs/vfs.h"

size_t proc_fd_read(vfs_file_t* file, size_t offset, size_t size, void* ptr)
{
    vfs_file_t* real = file->metadata;
    vfs_seek(real, offset);
    return vfs_read(real, size, ptr);
}
