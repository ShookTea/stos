#include "debugger.h"

#include <kernel/drivers/keyboard.h>
#include <stdio.h>
#include <kernel/tty.h>
#include <string.h>
#include <kernel/pmm.h>
#include <kernel/paging.h>
#include <kernel/vmm.h>
#include <kernel/multiboot2.h>

#define MAX_COMMAND_LENGTH 64

static char command_buffer[MAX_COMMAND_LENGTH];
static uint8_t command_length = 0;
static bool accept_commands = false;

static void print_prompt_and_enable()
{
    printf("# ");
    tty_enable_cursor(0, 15);
    command_length = 0;
    memset(command_buffer, 0, MAX_COMMAND_LENGTH);
    accept_commands = true;
}

static void handle_command_sent()
{
    // First, disable input
    accept_commands = false;
    tty_disable_cursor();

    puts("");
    if (strncmp(command_buffer, "help", 4) == 0) {
        puts("Available commands:");
        puts("  mb2_stats      - Prints GRUB multiboot2 stats");
        puts("  pag_stats      - Prints paging stats");
        puts("  pmm_stats      - Prints physical memory statistics");
        puts("  vmm_memory_map - Prints detailed memory map");
        puts("  vmm_stats      - Prints virtual memory statistics");
    }
    else if (strncmp(command_buffer, "mb2_stats", 9) == 0) {
        multiboot2_print_stats();
    }
    else if (strncmp(command_buffer, "pag_stats", 9) == 0) {
        paging_print_stats();
    }
    else if (strncmp(command_buffer, "pmm_stats", 9) == 0) {
        pmm_print_stats();
    }
    else if (strncmp(command_buffer, "vmm_memory_map", 14) == 0) {
        vmm_print_memory_map();
    }
    else if (strncmp(command_buffer, "vmm_stats", 9) == 0) {
        vmm_print_stats();
    }
    else {
        printf("Unrecognized command: %s\n", command_buffer);
    }

    // Re-enable prompt
    print_prompt_and_enable();
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
    } else if (evt.key_code == KCODE_ENTER
        || evt.key_code == KCODE_NUMPAD_ENTER) {
            handle_command_sent();
    } else if (command_length < (MAX_COMMAND_LENGTH - 1)) {
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
