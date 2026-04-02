#include "kernel/drivers/keyboard.h"
#include "kernel/task/wait.h"
#include "kernel/vfs/device.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/vfs/vfs.h"
#include <stdio.h>
#include <string.h>

static vfs_node_t* node = NULL;
static wait_obj_t* wait_obj;

static char* buffer = NULL;
static size_t buffer_length = 0;
static bool buffer_ready = false;

static void handle_key_event(keyboard_event_t evt)
{
    if (wait_obj->count == 0) {
        // There are no tasks reading this file right now
        return;
    }
    if (buffer_ready || !evt.pressed || !evt.ascii) {
        // TODO: handle cursor moving left and right
        return;
    }

    if (evt.key_code == KCODE_BACKSPACE) {
        if (buffer_length > 0) {
            buffer[buffer_length - 1] = 0;
            buffer_length--;
            putchar('\b');
        }
    } else if (evt.key_code == KCODE_ENTER
        || evt.key_code == KCODE_NUMPAD_ENTER) {
            putchar('\n');
            // Add null character
            buffer = krealloc(buffer, sizeof(char) * (buffer_length + 1));
            buffer[buffer_length] = '\0';
            buffer_length++;
            // Mark buffer as ready
            buffer_ready = true;
            // Wake up next task in the queue
            wait_wake_up(wait_obj);
    } else {
        buffer = krealloc(buffer, sizeof(char) * (buffer_length + 1));
        buffer[buffer_length] = evt.ascii;
        buffer_length++;
        putchar(evt.ascii);
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
    if (read_end > buffer_length) {
        read_end = buffer_length;
    }
    memcpy(ptr, buffer, read_end);
    kfree(buffer);
    buffer_length = 0;
    buffer_ready = false;
    return read_end;
}

vfs_node_t* device_tty_mount()
{
    if (node != NULL) {
        return node;
    }

    node = kmalloc(sizeof(vfs_node_t));
    vfs_populate_node(node, "tty", VFS_TYPE_CHARACTER_DEVICE);
    node->read_node = read;

    wait_obj = wait_allocate_queue(); // TODO: implement unmount() and free
    keyboard_register_listener(handle_key_event);
    return node;
}
