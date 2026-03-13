#include "debugger.h"

#include <kernel/drivers/keyboard.h>
#include <stdio.h>
#include <kernel/tty.h>

#define MAX_COMMAND_LENGTH 64

static char command_buffer[MAX_COMMAND_LENGTH];
static uint8_t command_length = 0;
static bool accept_commands = false;

static void print_prompt_and_enable()
{
    printf("# ");
    tty_enable_cursor(0, 15);
    accept_commands = true;
}

static void handle_key_event(keyboard_event_t evt)
{
    if (!accept_commands || !evt.pressed || !evt.ascii) {
        // TODO: handle cursor moving left and right
        return;
    }
    if (evt.key_code == KCODE_BACKSPACE) {
        if (command_length > 0) {
            // Backspace moves cursor one step to the left, so sending space
            // and another backspace effectively removes the character to the
            // left.
            tty_writestring("\b \b");
            command_length--;
        }
    } else if (command_length < MAX_COMMAND_LENGTH) {
        putchar(evt.ascii);
        command_buffer[command_length] = evt.ascii;
        command_length++;
    }

}
void debugger_init()
{
    keyboard_register_listener(handle_key_event);
    puts("");
    puts("");
    puts("Kernel debugger. Write \"help\" to get list of available commands.");
    print_prompt_and_enable();
}
