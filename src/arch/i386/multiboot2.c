#include "kernel/pmm.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <kernel/multiboot2.h>
#include <kernel/acpi.h>

static multiboot_info_t* mbi;
static char boot_command_line[64];
static char boot_loader_name[64];
static uint32_t load_base_addr = 0;

// Memory map storage
#define MAX_MMAP_ENTRIES 16

static saved_mmap_entry_t saved_mmap[MAX_MMAP_ENTRIES];
static uint32_t saved_mmap_count = 0;
static uint32_t pmm_max_memory = 0;  // Maximum usable memory address

static void multiboot2_process_memory_map()
{
    if (saved_mmap_count == 0) {
        printf("WARNING: No memory map entries found\n");
        return;
    }

    printf("Memory map entries: %u\n", saved_mmap_count);

    // Find the maximum usable memory address
    pmm_max_memory = 0;
    for (uint32_t i = 0; i < saved_mmap_count; i++) {
        saved_mmap_entry_t* entry = &saved_mmap[i];

        // Only consider available RAM (type 1)
        if (entry->type == 1) {
            // Calculate end address (base + length)
            uint64_t base = ((uint64_t)entry->base_high << 32) |
                           entry->base_low;
            uint64_t length = ((uint64_t)entry->length_high << 32) |
                             entry->length_low;
            uint64_t end = base + length;

            // We only support 32-bit addressing for now
            if (end > 0xFFFFFFFF) {
                end = 0xFFFFFFFF;
            }

            if ((uint32_t)end > pmm_max_memory) {
                pmm_max_memory = (uint32_t)end;
            }
        }
    }

    printf("Maximum memory address: %#x (%u MB)\n",
           pmm_max_memory, pmm_max_memory / (1024 * 1024));
}

void multiboot2_init(multiboot_info_t* multiboot_info)
{
    mbi = multiboot_info;
    uint64_t mb2_end = mbi->total_size + (uint32_t)mbi;
    // adding 8 bytes here to adjust for the multiboot_info_t header
    multiboot_tag_t* tag = (multiboot_tag_t*)((uint8_t*)mbi + 8);

    while ((uint32_t)tag < mb2_end) {
        switch (tag->type) {
            case MULTIBOOT2_TAG_TYPE_END:
                // We've reached the END tag, process memory map
                multiboot2_process_memory_map();
                return;
            case MULTIBOOT2_TAG_TYPE_BOOT_COMMAND_LINE: {
                multiboot_tag_boot_command_line_t* cl =
                    (multiboot_tag_boot_command_line_t*)tag;
                size_t len = strlen(cl->command_line);
                if (len > 64) len = 64;
                memcpy(boot_command_line, cl->command_line, len);
                break;
            }
            case MULTIBOOT2_TAG_TYPE_BOOT_LOADER_NAME: {
                multiboot_tag_boot_loader_name_t* bln =
                    (multiboot_tag_boot_loader_name_t*)tag;
                size_t len = strlen(bln->name);
                if (len > 64) len = 64;
                memcpy(boot_loader_name, bln->name, len);
                break;
            }
            case MULTIBOOT2_TAG_TYPE_LOAD_BASE_ADDR:
                load_base_addr = ((multiboot_tag_load_base_addr_t*)tag)
                    ->load_base_addr;
                break;
            case MULTIBOOT2_TAG_TYPE_MEMORY_MAP: {
                multiboot_tag_memory_map_t* mmap =
                    (multiboot_tag_memory_map_t*)tag;
                uint32_t entries_count = (mmap->size - 16) / mmap->entry_size;

                if (entries_count > MAX_MMAP_ENTRIES) {
                    printf("WARNING: Too many memory map entries, truncating to %d\n",
                           MAX_MMAP_ENTRIES);
                    entries_count = MAX_MMAP_ENTRIES;
                }

                // Save memory map entries
                saved_mmap_count = entries_count;
                for (uint32_t i = 0; i < entries_count; i++) {
                    multiboot_tag_memory_map_entry_t* entry = &mmap->entries[i];
                    saved_mmap[i].base_low = entry->base_addr_low;
                    saved_mmap[i].base_high = entry->base_addr_high;
                    saved_mmap[i].length_low = entry->length_low;
                    saved_mmap[i].length_high = entry->length_high;
                    saved_mmap[i].type = entry->type;
                }
                break;
            }
            case MULTIBOOT2_TAG_TYPE_ACPI_OLD: {
                multiboot_tag_boot_acpi_old_t* acpi =
                    (multiboot_tag_boot_acpi_old_t*)tag;
                acpi_init_old(acpi->rsdp);
                break;
            }
            case MULTIBOOT2_TAG_TYPE_ACPI_NEW: {
                multiboot_tag_boot_acpi_new_t* acpi =
                    (multiboot_tag_boot_acpi_new_t*)tag;
                acpi_init_new(acpi->rsdp);
                break;
            }
            default:
                // Unknown multiboot2 mode
                break;
        }

        // Iterate to the next tag
        uintptr_t next = ((uintptr_t)tag + tag->size + 7) & ~7ULL;
        tag = (multiboot_tag_t*)next;
    }

    // If we get here without finding END tag, still process memory map
    multiboot2_process_memory_map();
}

uint32_t multiboot2_get_max_memory()
{
    return pmm_max_memory;
}

uint32_t multiboot2_get_mmap_count()
{
    return saved_mmap_count;
}

const saved_mmap_entry_t* multiboot2_get_mmap_entry(uint32_t index)
{
    if (index >= saved_mmap_count) {
        return NULL;
    }
    return &saved_mmap[index];
}

void multiboot2_print_stats()
{
    printf("Bootloader name: %s\n", boot_loader_name);
    printf("Boot command: %s\n", boot_command_line);
    printf("Load base address: %#x\n", load_base_addr);
    puts("Memory map:");
    for (uint32_t i = 0; i < multiboot2_get_mmap_count(); i++) {
        const saved_mmap_entry_t* entry = multiboot2_get_mmap_entry(i);
        char* type = "????";
        switch (entry->type) {
            case 1:
                type = "avlb"; // Available
                break;
            case 2:
                type = "rsvd"; // Reserved
                break;
            case 3:
                type = "acpi"; // ACPI reclaimable
                break;
            case 4:
                type = " nvs"; // NVS
                break;
            case 5:
                type = "bram"; // bad RAM
                break;
        }

        uint64_t base = ((uint64_t)entry->base_high << 32) |
                       entry->base_low;
        uint64_t length = ((uint64_t)entry->length_high << 32) |
                         entry->length_low;
        uint64_t end = base + length;
        printf(
            "  [%03d] %s %#016llx : %#016llx (%d KiB)\n",
            i,
            type,
            base,
            end,
            length / 1024
        );
    }
}
