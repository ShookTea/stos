#include "./pipe.h"
#include "fcntl.h"
#include "kernel/memory/kmalloc.h"
#include <errno.h>

int pipe_open(vfs_node_t* node, vfs_file_t* file, uint8_t mode)
{
    if (node == NULL || file == NULL) {
        return EIO;
    }

    if (mode & O_RDWR) {
        return EIO;
    }

    pipe_node_meta_t* node_meta = node->metadata;
    bool read = mode & O_RDONLY;

    // check if already opened
    if (read && node_meta->read_opened) {
        return EIO;
    }
    if (!read && node_meta->write_opened) {
        return EIO;
    }

    if (read) {
        node_meta->read_opened = true;
    } else {
        node_meta->write_opened = true;
    }

    pipe_file_meta_t* file_meta = kmalloc_flags(
        sizeof(pipe_file_meta_t),
        KMALLOC_ZERO
    );
    file_meta->read_side = read;
    file->metadata = file_meta;

    return 0;
}
