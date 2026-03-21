#include "debugger.h"

#include <kernel/drivers/keyboard.h>
#include <stdio.h>
#include <kernel/tty.h>
#include <stdlib.h>
#include <string.h>
#include <kernel/memory/pmm.h>
#include <kernel/memory/vmm.h>
#include <kernel/memory/slab.h>
#include <kernel/memory/kmalloc.h>
#include <kernel/paging.h>
#include <kernel/multiboot2.h>
#include <kernel/acpi.h>
#include <kernel/vfs/vfs.h>
#include "test/memory_leak_tests.h"
#include "test/memory_tests.h"
#include "test/vmm_tests.h"
#include "test/kmalloc_tests.h"
#include "test/libc_tests.h"

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
        puts("  libc_test      - Runs libc test suite");
        puts("  mb2_data       - Prints GRUB multiboot2 data");
        puts("  memleak_test   - Runs memleak test");
        puts("  pag_stats      - Prints paging stats");
        puts("  pmm_stats      - Prints physical memory statistics");
        puts("  pmm_test       - Runs physical memory test suite");
        puts("  reboot         - Reboot the system via ACPI");
        puts("  shutdown       - Shutdown the system via ACPI");
        puts("  slab_cache     - Prints slab allocator cache info");
        puts("  slab_stats     - Prints slab allocator statistics");
        puts("  vfs_cat [F]    - Prints content of a file at abs. path");
        puts("  vfs_ls [F]     - Prints info about file at abs. path [F]");
        puts("  vmm_memory_map - Prints detailed memory map");
        puts("  vmm_stats      - Prints virtual memory statistics");
        puts("  vmm_test       - Runs virtual memory test suite");
    }
    else if (strcmp(command, "kmalloc_a") == 0) {
        if (argcount != 1) {
            puts("kmalloc_a requires 1 argument");
        } else {
            int memsize = atoi(args[0]);
            uint32_t addr = (uint32_t) kmalloc(memsize);
            printf("Allocated %d B at address %d\n", memsize, addr);
        }
    }
    else if (strcmp(command, "kmalloc_f") == 0) {
        if (argcount != 1) {
            puts("kmalloc_f requires 1 argument");
        } else {
            void* addr = (void*)atoi(args[0]);
            size_t size = kmalloc_size(addr);
            kfree(addr);
            printf("Freed %d bytes from address %d\n", size, addr);
        }
    }
    else if (strcmp(command, "kmalloc_stats") == 0) {
        kmalloc_print_stats();
    }
    else if (strcmp(command, "kmalloc_test") == 0) {
        kmalloc_run_all_tests();
    }
    else if (strcmp(command, "libc_test") == 0) {
        libc_run_all_tests();
    }
    else if (strcmp(command, "mb2_data") == 0) {
        multiboot2_print_data();
    }
    else if (strcmp(command, "memleak_test") == 0) {
        memory_leak_run_test();
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
    else if (strcmp(command, "vfs_cat") == 0) {
        if (argcount != 1) {
            puts("vfs_cat requires 1 argument");
        } else {
            vfs_node_t* node = vfs_resolve(args[0]);
            if (node == NULL) {
                puts("File or directory not found.");
            } else if ((node->type & VFS_TYPE_FILE) == 0) {
                puts("Found node, but it's not a file.");
            } else {
                char* buffer = kmalloc_flags(sizeof(char) * 17, KMALLOC_ZERO);
                vfs_file_t* handle = vfs_open(node, VFS_MODE_READONLY);
                size_t read_bytes;
                while ((read_bytes = vfs_read(handle, 16, buffer)) != 0) {
                    printf("%s", buffer);
                }

                vfs_close(handle);
                kfree(buffer);
            }
        }
    }
    else if (strcmp(command, "vfs_ls") == 0) {
        if (argcount != 1) {
            puts("vfs_ls requires 1 argument");
        } else {
            vfs_node_t* node = vfs_resolve(args[0]);
            if (node == NULL) {
                puts("File or directory not found.");
            } else if (node->type & VFS_TYPE_DIRECTORY) {
                puts("Directory found. Children:");
                size_t i = 0;
                struct dirent* dir = vfs_readdir(node, i);
                while (dir != NULL) {
                    vfs_node_t* child = vfs_finddir(node, dir->name);
                    i++;
                    dir = vfs_readdir(node, i);

                    if (child->type & VFS_TYPE_DIRECTORY) {
                        printf(" - %s (dir)\n", child->filename);
                    } else {
                        printf(
                            " - %s (%d B)\n",
                            child->filename,
                            child->length
                        );
                    }
                }
            } else if (node->type & VFS_TYPE_FILE) {
                puts("File found");
            } else {
                puts("Unknown vfs node found");
            }
        }
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
    else if (strcmp(command, "shutdown") == 0) {
        puts("Initiating ACPI shutdown...");
        acpi_shutdown();
        // Never returns if successful
    }
    else if (strcmp(command, "reboot") == 0) {
        puts("Initiating ACPI reboot...");
        acpi_reboot();
        // Never returns if successful
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
