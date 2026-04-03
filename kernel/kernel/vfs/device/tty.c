#include "kernel/drivers/keyboard.h"
#include "kernel/task/wait.h"
#include "kernel/vfs/device.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/vfs/vfs.h"
#include "libds/ringbuf.h"
#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE 4096

static vfs_node_t* node = NULL;
static wait_obj_t* wait_obj;

static ds_ringbuf_t* buffer;
static bool buffer_ready = false;

static void handle_key_event(keyboard_event_t evt)
{
    // if (wait_obj->count == 0) {
        // There are no tasks reading this file right now
        // return;
    // }
    if (buffer_ready || !evt.pressed || !evt.ascii) {
        // TODO: handle cursor moving left and right
        return;
    }

    if (evt.key_code == KCODE_ENTER
        || evt.key_code == KCODE_NUMPAD_ENTER) {
            char nline = '\n';
            putchar(nline);
            ds_ringbuf_push(buffer, &nline);

            if (wait_obj->count > 0) {
                buffer_ready = true;
                wait_wake_up(wait_obj);
            }
    } else {
        putchar(evt.ascii);
        ds_ringbuf_push(buffer, &(evt.ascii));
    }
}

static bool is_buffer_ready()
{
    return buffer_ready;
}

static size_t read(
    vfs_file_t* file __attribute__((unused)),
    size_t offset __attribute__((unused)),
    size_t size,
    void* ptr
) {
    puts("Entering condition loop");
    wait_on_condition(wait_obj, is_buffer_ready, NULL);
    size_t read_end = size;
    if (read_end > ds_ringbuf_size(buffer)) {
        read_end = ds_ringbuf_size(buffer);
    }
    for (size_t i = 0; i <= read_end; i++) {
        ds_ringbuf_pop(buffer, (uint8_t*)ptr + i);
    }
    ds_ringbuf_clear(buffer);
    buffer_ready = false;
    return read_end;
}

vfs_node_t* device_tty_mount()
{
    if (node != NULL) {
        return node;
    }

    buffer = ds_ringbuf_create(BUFFER_SIZE, sizeof(char), false);
    node = kmalloc(sizeof(vfs_node_t));
    vfs_populate_node(node, "tty", VFS_TYPE_CHARACTER_DEVICE);
    node->read_node = read;

    wait_obj = wait_allocate_queue();
    keyboard_register_listener(handle_key_event);
    return node;
}

void device_tty_unmount()
{
    if (node != NULL) {
        kfree(node);
        node = NULL;
    }
    if (wait_obj != NULL) {
        wait_deallocate(wait_obj);
        wait_obj = NULL;
    }
    if (buffer != NULL) {
        ds_ringbuf_destroy(buffer);
        buffer = NULL;
    }
}
