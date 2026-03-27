#include "debugger.h"

#include <kernel/drivers/keyboard.h>
#include <stdio.h>
#include <kernel/terminal.h>
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
#include <kernel/elf.h>
#include <kernel/task/scheduler.h>
#include <kernel/task/task.h>
#include "test/memory_leak_tests.h"
#include "test/memory_tests.h"
#include "test/vmm_tests.h"
#include "test/kmalloc_tests.h"
#include "test/libc_tests.h"

#define MAX_COMMAND_LENGTH 64

static char command_buffer[MAX_COMMAND_LENGTH];
static uint8_t command_length = 0;
static bool accept_commands = false;
static uint32_t background_task_count = 0;

static void background_task()
{
    background_task_count++;
    uint32_t task_id = background_task_count;
    uint32_t counter1 = 0;
    uint32_t counter2 = 0;
    while (1) {
        counter1++;
        if (counter1 % 20 == 0) {
            counter2++;
            printf("Task %u - counter %u\n", task_id, counter2);
        }
        if (counter2 == 20) {
            break;
        }
        // Busy wait to let time pass
        for (volatile int i = 0; i < 10000000; i++);
    }
}

static void print_prompt_and_enable()
{
    puts("");
    printf("\033[36;1m#\033[0m ");
    terminal_enable_cursor();
    command_length = 0;
    memset(command_buffer, 0, MAX_COMMAND_LENGTH);
    accept_commands = true;
}

static void handle_command_sent()
{
    // First, disable input
    accept_commands = false;
    terminal_disable_cursor();
    puts("");

    if (command_length == 0 || command_buffer[0] == '\0') {
        print_prompt_and_enable();
        return;
    }

    char* args[3];
    char* arg = strtok(command_buffer, " ");
    char* command = arg;

    if (command == NULL) {
        print_prompt_and_enable();
        return;
    }

    uint8_t argcount = 0;
    while ((arg = strtok(NULL, " ")) != NULL && argcount < 3) {
        args[argcount] = arg;
        argcount++;
    }
    if (strcmp(command, "help") == 0) {
        puts("Available commands:");
        puts("  elf_dump [F]   - Dumps info about ELF file at path [F]");
        puts("  exec [F]       - Runs executable ELF file at path [F]");
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
        puts("  ps             - Lists all currently registered tasks");
        puts("  reboot         - Reboot the system via ACPI");
        puts("  shutdown       - Shutdown the system via ACPI");
        puts("  slab_cache     - Prints slab allocator cache info");
        puts("  slab_stats     - Prints slab allocator statistics");
        puts("  task_run       - Launches a new task in the background");
        puts("  vfs_cat [F]    - Prints content of a file at abs. path");
        puts("  vfs_ls [F]     - Prints info about file at abs. path [F]");
        puts("  vga_colors     - Prints VGA colors map");
        puts("  vmm_memory_map - Prints detailed memory map");
        puts("  vmm_stats      - Prints virtual memory statistics");
        puts("  vmm_test       - Runs virtual memory test suite");
    }
    else if (strcmp(command, "elf_dump") == 0) {
        if (argcount != 1) {
            puts("elf_dump requires 1 argument");
        } else {
            vfs_node_t* node = vfs_resolve(args[0]);
            if (node == NULL) {
                puts("File or directory not found.");
            } else if ((node->type & VFS_TYPE_FILE) == 0) {
                puts("Found node, but it's not a file.");
            } else {
                vfs_file_t* handle = vfs_open(node, VFS_MODE_READONLY);
                elf_dump(handle);
            }
        }
    }
    else if (strcmp(command, "exec") == 0) {
        if (argcount != 1) {
            puts("exec requires 1 argument");
        } else {
            vfs_node_t* node = vfs_resolve(args[0]);
            if (node == NULL) {
                puts("File or directory not found.");
            } else if ((node->type & VFS_TYPE_FILE) == 0) {
                puts("Found node, but it's not a file.");
            } else {
                vfs_file_t* handle = vfs_open(node, VFS_MODE_READONLY);
                void* file = kmalloc_flags(handle->node->length, KMALLOC_ZERO);
                vfs_read(handle, handle->node->length, file);
                vfs_close(handle);
                elf_create_task(node->filename, file);
                kfree(file);
            }
        }
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
    else if (strcmp(command, "ps") == 0) {
        size_t tasks = task_get_tasks_count();
        printf("%d tasks found:\n", tasks);
        for (size_t i = 0; i < tasks; i++) {
            task_t* task = task_get_task_by_index(i);
            char* status;
            switch (task->state) {
                case TASK_WAITING: status = "waiting"; break;
                case TASK_RUNNING: status = "running"; break;
                case TASK_BLOCKED: status = "blocked"; break;
                case TASK_SLEEPING: status = "sleeping"; break;
                case TASK_ZOMBIE: status = "zombie"; break;
                case TASK_DEAD: status = "dead"; break;
                default: status = "???"; break;
            }
            printf(
                "-[%u] '%s' (status: '%s')\n",
                task->pid,
                task->name,
                status
            );
        }
    }
    else if (strcmp(command, "slab_cache") == 0) {
        slab_print_caches();
    }
    else if (strcmp(command, "slab_stats") == 0) {
        slab_print_stats();
    }
    else if (strcmp(command, "task_run") == 0) {
        task_t* task = task_create("test_task_run", background_task, true);
        scheduler_add_task(task);
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
                    memset(buffer, 0, 17);
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
    else if (strcmp(command, "vga_colors") == 0) {
        puts("Color matrix with styling using ^[F;Bm");
        puts("");
        // Print header with bg colors
        for (uint8_t bg_color = 39; bg_color <= 47; bg_color++) {
            if (bg_color == 39) {
                printf("    ");
            }
            else {
                printf(" %d ", bg_color);
            }
        }
        for (uint8_t bg_color = 100; bg_color <= 107; bg_color++) {
                printf(" %d", bg_color);
        }
        puts("");

        // Print rows
        for (uint8_t fg_color = 30; fg_color <= 37; fg_color++) {
            printf(" %d ", fg_color);
            for (uint8_t bg_color = 40; bg_color <= 47; bg_color++) {
                printf(
                    "\033[%d;%dm%s",
                    fg_color,
                    bg_color,
                    " ## "
                );
            }
            for (uint8_t bg_color = 100; bg_color <= 107; bg_color++) {
                printf(
                    "\033[%d;%dm%s",
                    fg_color,
                    bg_color,
                    " ## "
                );
            }
            puts("\033[m");
        }
        for (uint8_t fg_color = 90; fg_color <= 97; fg_color++) {
            printf(" %d ", fg_color);
            for (uint8_t bg_color = 40; bg_color <= 47; bg_color++) {
                printf(
                    "\033[%d;%dm%s",
                    fg_color,
                    bg_color,
                    " ## "
                );
            }
            for (uint8_t bg_color = 100; bg_color <= 107; bg_color++) {
                printf(
                    "\033[%d;%dm%s",
                    fg_color,
                    bg_color,
                    " ## "
                );
            }
            puts("\033[m");
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
            terminal_write_char('\b');
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

    // Entering idle loop - debugger should run forever
    while (1) {
        __asm__ volatile("hlt");
    };
}
