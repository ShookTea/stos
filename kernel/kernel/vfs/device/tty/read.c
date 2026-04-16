#include <stddef.h>
#include "kernel/vfs/vfs.h"
#include "./tty.h"

static bool is_buffer_ready(void* arg)
{
    tty_state_t* meta = arg;
    if (meta->lflag & TTY_LFLAG_ICANON) {
        return meta->ready_lines > 0;
    } else {
        return ds_ringbuf_size(meta->buffer) > 0;
    }
}

size_t tty_read(
    vfs_file_t* file,
    size_t offset __attribute__((unused)),
    size_t size,
    void* ptr
) {
    tty_state_t* meta = file->dentry->inode->metadata;
    wait_on_condition(meta->wait_obj, is_buffer_ready, meta);
    size_t read_bytes = 0;
    size_t buffer_size = ds_ringbuf_size(meta->buffer);
    if (buffer_size < size) {
        size = buffer_size;
    }

    for (size_t i = 0; i < size; i++) {
        char c;
        ds_ringbuf_pop(meta->buffer, &c);
        *((uint8_t*)ptr + i) = c;
        read_bytes++;
        if ((meta->lflag & TTY_LFLAG_ICANON) && c == '\n') {
            // TODO: if previous character was non-escaped backslash, that means
            // the new line character is a part of line
            meta->ready_lines--;
            break;
        }
    }
    return read_bytes;
}
