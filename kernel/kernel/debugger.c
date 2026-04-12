#include "debugger.h"
#include <stdint.h>
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
#include "kernel/drivers/ata.h"
#include "test/memory_leak_tests.h"
#include "test/memory_tests.h"
#include "test/vmm_tests.h"
#include "test/kmalloc_tests.h"
#include "test/libc_tests.h"

#define MAX_COMMAND_LENGTH 64

static char command_buffer[MAX_COMMAND_LENGTH];
static uint8_t command_length = 0;

static void command_ata_dump_drive(uint8_t drive_id)
{
    uint32_t sectors = ata_get_lba28_sectors_count(drive_id);
    uint32_t mib = (sectors / 2) / 1024;
    ata_disk_info_t disk_info;
    if (!ata_load_disk_info(drive_id, &disk_info)) {
        puts("  Failed to load partition info.");
        return;
    }
    printf("  Firmware: %s\n", disk_info.firmare_name);
    printf("  Sectors count: %u (%u MiB)\n", sectors, mib);
    printf("  %u partitions present:\n", disk_info.partitions_count);
    puts("  ┌────┬───────┬────────────┬────────────┬────────────┬────────────┐");
    puts("  │ Id │ Boot? │       Type │  LBA start │    Sectors │ Size (MiB) │");
    puts("  ├────┼───────┼────────────┼────────────┼────────────┼────────────┤");
    for (size_t i = 0; i < disk_info.partitions_count; i++) {
        ata_partition_t part = disk_info.partitions[i];
        char* type
            = part.type == ATA_PARTITION_TYPE_FAT32 ? "FAT-32"
            : part.type == ATA_PARTITION_TYPE_LINUX_NATIVE ? "Linux"
            : "unknown";
        printf(
            "  │ %2u │ %5s │ %10s │ %10d │ %10d | %10d |\n",
            i,
            part.bootable ? " yes " : "",
            type,
            part.lba_start,
            part.sectors_count,
            part.sectors_count / 2048
        );
    }
    puts("  └────┴───────┴────────────┴────────────┴────────────┴────────────┘");
}

static void command_ata_dump()
{
    if (ata_drive_available(ATA_DRIVE_PRIMARY_MASTER)) {
        puts("ATA drive primary/master available");
        command_ata_dump_drive(ATA_DRIVE_PRIMARY_MASTER);
    } else {
        puts("ATA drive primary/master not present");
    }
    if (ata_drive_available(ATA_DRIVE_PRIMARY_SLAVE)) {
        puts("ATA drive primary/slave available");
        command_ata_dump_drive(ATA_DRIVE_PRIMARY_SLAVE);
    } else {
        puts("ATA drive primary/slave not present");
    }
    if (ata_drive_available(ATA_DRIVE_SECONDARY_MASTER)) {
        puts("ATA drive secondary/master available");
        command_ata_dump_drive(ATA_DRIVE_SECONDARY_MASTER);
    } else {
        puts("ATA drive secondary/master not present");
    }
    if (ata_drive_available(ATA_DRIVE_SECONDARY_SLAVE)) {
        puts("ATA drive secondary/slave available");
        command_ata_dump_drive(ATA_DRIVE_SECONDARY_SLAVE);
    } else {
        puts("ATA drive secondary/slave not present");
    }
}

static void handle_command_sent()
{
    puts("");

    if (command_length == 0 || command_buffer[0] == '\0') {
        return;
    }

    char* args[3];
    char* arg = strtok(command_buffer, " ");
    char* command = arg;

    if (command == NULL) {
        return;
    }

    uint8_t argcount = 0;
    while ((arg = strtok(NULL, " ")) != NULL && argcount < 3) {
        args[argcount] = arg;
        argcount++;
    }
    if (strcmp(command, "help") == 0) {
        puts("Available commands:");
        puts("  ata_dump       - Dumps information from ATA driver");
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
        puts("  vfs_cat [F]    - Prints content of a file at abs. path");
        puts("  vfs_ls [F]     - Prints info about file at abs. path [F]");
        puts("  vga_colors     - Prints VGA colors map");
        puts("  vga_utf8 [N]   - Dumps Nth page of UTF-8, starting from 0");
        puts("  vmm_memory_map - Prints detailed memory map");
        puts("  vmm_stats      - Prints virtual memory statistics");
        puts("  vmm_test       - Runs virtual memory test suite");
    }
    else if (strcmp(command, "ata_dump") == 0) {
        command_ata_dump();
    }
    else if (strcmp(command, "elf_dump") == 0) {
        if (argcount != 1) {
            puts("elf_dump requires 1 argument");
        } else {
            dentry_t* node = vfs_resolve(args[0]);
            if (node == NULL) {
                puts("File or directory not found.");
            } else if ((node->inode->type & VFS_TYPE_FILE) == 0) {
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
            dentry_t* node = vfs_resolve(args[0]);
            if (node == NULL) {
                puts("File or directory not found.");
            } else if ((node->inode->type & VFS_TYPE_FILE) == 0) {
                puts("Found node, but it's not a file.");
            } else {
                dentry_t* root_dir = vfs_get_real_root();
                vfs_file_t* handle = vfs_open(node, VFS_MODE_READONLY);
                void* file = kmalloc_flags(
                    handle->dentry->inode->length,
                    KMALLOC_ZERO
                );
                vfs_read(handle, handle->dentry->inode->length, file);
                vfs_close(handle);
                task_t* task = elf_create_task(
                    node->name,
                    file,
                    root_dir,
                    root_dir
                );
                kfree(file);
                scheduler_add_task(task);
                printf("Task [%u] '%s' scheduled.\n", task->pid, task->name);
                int status;
                task_wait(task->pid, &status);
                printf("Task completed with status %d\n", status);
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
        multiboot2_print_data(true);
    }
    else if (strcmp(command, "memleak_test") == 0) {
        memory_leak_run_test();
    }
    else if (strcmp(command, "pag_stats") == 0) {
        paging_print_stats(true);
    }
    else if (strcmp(command, "pmm_stats") == 0) {
        pmm_print_stats(true);
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
                "-[%u] (parent: %d) '%s' (status: '%s')\n",
                task->pid,
                task->parent == NULL ? 0 : task->parent->pid,
                task->name,
                status
            );
        }
    }
    else if (strcmp(command, "slab_cache") == 0) {
        slab_print_caches(true);
    }
    else if (strcmp(command, "slab_stats") == 0) {
        slab_print_stats(true);
    }
    else if (strcmp(command, "vfs_cat") == 0) {
        if (argcount != 1) {
            puts("vfs_cat requires 1 argument");
        } else {
            dentry_t* node = vfs_resolve(args[0]);
            if (node == NULL) {
                puts("File or directory not found.");
            } else if ((node->inode->type & VFS_TYPE_FILE) == 0) {
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
            dentry_t* node = vfs_resolve(args[0]);
            if (node == NULL) {
                puts("File or directory not found.");
            } else if (node->inode->type & VFS_TYPE_DIRECTORY) {
                puts("Directory found. Children:");
                size_t i = 0;
                struct dirent dir;
                while (vfs_readdir(node, i, &dir)) {
                    dentry_t* child = vfs_finddir(node, dir.name);
                    i++;

                    if (child->inode->type & VFS_TYPE_DIRECTORY) {
                        printf(" - %s (dir)\n", child->name);
                    } else {
                        double length = child->inode->length;
                        const char* unit;
                        bool use_byte = false;
                        if (length < 2048) {
                            unit = "B";
                            use_byte = true;
                        } else {
                            length /= 1024;
                            if (length < 2048) {
                                unit = "KiB";
                            } else {
                                length /= 1024;
                                if (length < 2048) {
                                    unit = "MiB";
                                } else {
                                    length /= 1024;
                                    unit = "GiB";
                                }
                            }
                        }
                        printf(
                            use_byte ? "  %s (%llu %s)\n" : "  %s (%.1f %s)\n",
                            child->name,
                            use_byte ? child->inode->length : length,
                            unit
                        );
                    }
                }
            } else if (node->inode->type & VFS_TYPE_FILE) {
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
    else if (strcmp(command, "vga_utf8") == 0) {
        if (argcount != 1) {
            puts("vga_utf8 requires 1 argument");
        } else {
            int page = atoi(args[0]);
            if (page < 0) {
                puts("Page must be greater or equal to 0");
            } else {
                uint32_t page_size = 0x80;
                uint32_t first_char = page * page_size;
                uint32_t last_char = ((page + 1) * page_size) - 1;
                printf(
                    "Page %d selected (U-%04X - U-%04X)",
                    page,
                    first_char,
                    last_char
                );

                for (uint32_t i = 0; i < page_size; i++) {
                    if (i % 16 == 0) {
                        puts("");
                    }
                    uint32_t utf8_value = first_char + i;
                    uint8_t byte_buf[4];
                    uint8_t byte_buf_size = 0;
                    if (utf8_value <= 0x7F) {
                        // ASCII
                        byte_buf_size = 1;
                        byte_buf[0] = utf8_value;
                    } else if (utf8_value <= 0x7FF) {
                        // 2-byte UTF-8
                        byte_buf_size = 2;
                        byte_buf[1] = 0x80 | (utf8_value & 0x3F);
                        utf8_value >>= 6;
                        byte_buf[0] = utf8_value | 0xC0;
                    } else if (utf8_value <= 0xFFFF) {
                        // 3-byte UTF-8
                        byte_buf_size = 3;
                        byte_buf[2] = 0x80 | (utf8_value & 0x3F);
                        utf8_value >>= 6;
                        byte_buf[1] = 0x80 | (utf8_value & 0x3F);
                        utf8_value >>= 6;
                        byte_buf[0] = utf8_value | 0xE0;
                    } else {
                        // 4-byte UTF-8
                        byte_buf_size = 4;
                        byte_buf[3] = 0x80 | (utf8_value & 0x3F);
                        utf8_value >>= 6;
                        byte_buf[2] = 0x80 | (utf8_value & 0x3F);
                        utf8_value >>= 6;
                        byte_buf[1] = 0x80 | (utf8_value & 0x3F);
                        utf8_value >>= 6;
                        byte_buf[0] = utf8_value | 0xF0;
                    }

                    printf(" ");
                    for (uint8_t j = 0; j < byte_buf_size; j++) {
                        putchar(byte_buf[j]);
                    }
                }
                puts("");
            }
        }
    }
    else if (strcmp(command, "vmm_memory_map") == 0) {
        vmm_print_memory_map(true);
    }
    else if (strcmp(command, "vmm_stats") == 0) {
        vmm_print_stats(true);
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
}

static void print_prompt_and_read_command()
{
    puts("");
    printf("\033[36;1m#\033[0m ");
    terminal_enable_cursor();

    dentry_t* tty = vfs_resolve("/dev/tty");
    vfs_file_t* handle = vfs_open(tty, VFS_MODE_READONLY);
    command_length = vfs_read(handle, MAX_COMMAND_LENGTH, command_buffer);
    command_buffer[command_length] = '\0';
    command_buffer[command_length - 1] = '\0'; // Clear new line character
    vfs_close(handle);

    terminal_disable_cursor();
    handle_command_sent();
}

void debugger_init()
{
    puts("Kernel debugger. Write \"help\" to get list of available commands.");

    // Entering I/O loop with reading command from /dev/tty and analyzing it.
    while (1) {
        print_prompt_and_read_command();
    }

    // Entering idle loop - debugger should run forever
    while (1) {
        __asm__ volatile("hlt");
    };
}
