#include "./pipe.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/task/wait.h"

void pipe_close(vfs_node_t* node, vfs_file_t* file)
{
    if (node == NULL || file == NULL) {
        return;
    }

    pipe_node_meta_t* node_meta = node->metadata;
    pipe_file_meta_t* file_meta = file->metadata;

    if (file_meta == NULL) {
        return;
    }

    if (file_meta->read_side) {
        if (node_meta->read_ref_count > 0) {
            node_meta->read_ref_count--;
        }
    } else {
        if (node_meta->write_ref_count > 0) {
            node_meta->write_ref_count--;
            if (node_meta->write_ref_count == 0) {
                wait_wake_up(node_meta->wait_obj);
            }
        }
    }

    kfree(file->metadata);
    file->metadata = NULL;
}
