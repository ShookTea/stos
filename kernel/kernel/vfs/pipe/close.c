#include "./pipe.h"
#include "kernel/memory/kmalloc.h"

void pipe_close(vfs_node_t* node, vfs_file_t* file)
{
    if (node == NULL || file == NULL) {
        return;
    }

    pipe_node_meta_t* node_meta = node->metadata;
    pipe_file_meta_t* file_meta = file->metadata;

    if (file_meta->read_side) {
        node_meta->read_closed = true;
    } else {
        node_meta->write_closed = true;
    }

    kfree(file->metadata);
    file->metadata = NULL;
}
