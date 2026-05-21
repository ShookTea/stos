#include "../proc.h"
#include "kernel/vfs/vfs.h"

void proc_fd_close(vfs_node_t* node __attribute__((unused)), vfs_file_t* file)
{
    vfs_file_t* real = file->metadata;
    vfs_close(real);
}
