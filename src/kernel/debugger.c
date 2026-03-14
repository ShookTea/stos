#include "debugger.h"

#include <kernel/drivers/keyboard.h>
#include <stdio.h>
#include <kernel/tty.h>
#include <string.h>
#include <kernel/memory/pmm.h>
#include <kernel/memory/vmm.h>
#include <kernel/memory/slab.h>
#include <kernel/memory/kmalloc.h>
#include <kernel/paging.h>
#include <kernel/multiboot2.h>
#include "test/memory_tests.h"
#include "test/vmm_tests.h"
#include "test/kmalloc_tests.h"

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
    char* args[3];
    char* arg = strtok(command_buffer, " ");
    char* command = arg;
    uint8_t argcount = 0;
    while ((arg = strtok(NULL, " ")) != NULL && argcount < 3) {
        args[argcount] = arg;
        argcount++;
    }
    if (strcmp(command, "help") == 0) {
        puts("Available commands:");
        puts("  kmalloc_a [N]  - allocates [N] bytes with kmalloc");
        puts("  kmalloc_f [AD] - frees address [AD] with kfree");
        puts("  kmalloc_stats  - Prints kmalloc statistics");
        puts("  kmalloc_test   - Prints kmalloc statistics");
        puts("  mb2_data       - Prints GRUB multiboot2 data");
        puts("  pag_stats      - Prints paging stats");
        puts("  pmm_stats      - Prints physical memory statistics");
        puts("  pmm_test       - Runs physical memory test suite");
        puts("  slab_cache     - Prints slab allocator cache info");
        puts("  slab_stats     - Prints slab allocator statistics");
        puts("  vmm_memory_map - Prints detailed memory map");
        puts("  vmm_stats      - Prints virtual memory statistics");
        puts("  vmm_test       - Runs virtual memory test suite");
    }
    else if (strcmp(command, "kmalloc_a") == 0) {
        if (argcount != 1) {
            puts("kmalloc_a requires 1 argument");
        }
    }
    else if (strcmp(command, "kmalloc_f") == 0) {
        if (argcount != 1) {
            puts("kmalloc_f requires 1 argument");
        }
    }
    else if (strcmp(command, "kmalloc_stats") == 0) {
        kmalloc_print_stats();
    }
    else if (strcmp(command, "kmalloc_test") == 0) {
        kmalloc_run_all_tests();
    }
    else if (strcmp(command, "mb2_data") == 0) {
        multiboot2_print_data();
    }
    else if (strcmp(command, "pag_stats") == 0) {
        paging_print_stats();
    }
    else if (strcmp(command, "pmm_stats") == 0) {
        pmm_print_stats();
    }
    else if (strcmp(command, "pmm_test") == 0) {
        memory_run_pmm_tests();
    }
    else if (strcmp(command, "slab_cache") == 0) {
        slab_print_caches();
    }
    else if (strcmp(command, "slab_stats") == 0) {
        slab_print_stats();
    }
    else if (strcmp(command, "vmm_memory_map") == 0) {
        vmm_print_memory_map();
    }
    else if (strcmp(command, "vmm_stats") == 0) {
        vmm_print_stats();
    }
    else if (strcmp(command, "vmm_test") == 0) {
        vmm_run_all_tests();
    }
    else {
        printf("Unrecognized command: %s\n", command);
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
            command_buffer[command_length] = 0;
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
