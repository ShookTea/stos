#include "debugger.h"

#include <kernel/drivers/keyboard.h>
#include <stdio.h>
#include <kernel/tty.h>

static char command_buffer[64];
static uint8_t command_length = 0;

static void handle_key_event(keyboard_event_t evt)
{
    if (evt.pressed && evt.ascii) {
        putchar(evt.ascii);
    }
}
void debugger_init()
{
    keyboard_register_listener(handle_key_event);
    puts("");
    puts("");
    puts("kernel debugger");
    printf("# ");
    tty_enable_cursor(0, 15);
}
