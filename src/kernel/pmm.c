#include <kernel/pmm.h>
#include <kernel/multiboot2.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// External symbols from linker script
extern uint32_t _kernel_start;
extern uint32_t _kernel_end;

// Bitmap to track page frames (each bit = one 4KB page)
static uint32_t* pmm_bitmap = NULL;
static uint32_t pmm_bitmap_size = 0;  // Size in uint32_t elements
static uint32_t pmm_total_pages = 0;
static uint32_t pmm_used_pages = 0;
static uint32_t pmm_max_memory = 0;    // Maximum usable memory address

// Saved memory map entries (to avoid corruption after bitmap initialization)
#define MAX_MMAP_ENTRIES 16
typedef struct {
    uint32_t base_low;
    uint32_t base_high;
    uint32_t length_low;
    uint32_t length_high;
    uint32_t type;
} saved_mmap_entry_t;

static saved_mmap_entry_t saved_mmap[MAX_MMAP_ENTRIES];
static uint32_t saved_mmap_count = 0;

// Helper functions for bitmap operations

/**
 * Set a bit in the bitmap (mark page as used)
 */
static inline void bitmap_set(uint32_t page)
{
    uint32_t index = page / 32;
    uint32_t bit = page % 32;
    pmm_bitmap[index] |= (1 << bit);
}

/**
 * Clear a bit in the bitmap (mark page as free)
 */
static inline void bitmap_clear(uint32_t page)
{
    uint32_t index = page / 32;
    uint32_t bit = page % 32;
    pmm_bitmap[index] &= ~(1 << bit);
}

/**
 * Test a bit in the bitmap
 * @return true if page is used, false if free
 */
static inline bool bitmap_test(uint32_t page)
{
    uint32_t index = page / 32;
    uint32_t bit = page % 32;
    return (pmm_bitmap[index] & (1 << bit)) != 0;
}

/**
 * Find the first free page in the bitmap
 * @return Page number, or (uint32_t)-1 if no free page found
 */
static uint32_t bitmap_find_free(void)
{
    for (uint32_t i = 0; i < pmm_bitmap_size; i++) {
        // Skip if all bits are set (all pages used)
        if (pmm_bitmap[i] == 0xFFFFFFFF) {
            continue;
        }

        // Find the first free bit in this uint32_t
        for (uint32_t bit = 0; bit < 32; bit++) {
            if (!(pmm_bitmap[i] & (1 << bit))) {
                uint32_t page = i * 32 + bit;
                if (page < pmm_total_pages) {
                    return page;
                }
            }
        }
    }
    return (uint32_t)-1;
}

/**
 * Find a contiguous block of free pages
 * @param count Number of contiguous pages needed
 * @return Starting page number, or (uint32_t)-1 if not found
 */
static uint32_t bitmap_find_free_pages(size_t count)
{
    if (count == 0) {
        return (uint32_t)-1;
    }

    if (count == 1) {
        return bitmap_find_free();
    }

    uint32_t free_count = 0;
    uint32_t start_page = 0;

    for (uint32_t page = 0; page < pmm_total_pages; page++) {
        if (!bitmap_test(page)) {
            if (free_count == 0) {
                start_page = page;
            }
            free_count++;

            if (free_count == count) {
                return start_page;
            }
        } else {
            free_count = 0;
        }
    }

    return (uint32_t)-1;
}

void pmm_init(multiboot_info_t* mbi)
{
    printf("Initializing Physical Memory Manager...\n");

    // Find the multiboot memory map tag
    multiboot_tag_memory_map_t* mmap_tag = NULL;
    uint64_t mb2_end = mbi->total_size + (uint32_t)mbi;
    multiboot_tag_t* tag = (multiboot_tag_t*)((uint8_t*)mbi + 8);

    while ((uint32_t)tag < mb2_end) {
        if (tag->type == MULTIBOOT2_TAG_TYPE_MEMORY_MAP) {
            mmap_tag = (multiboot_tag_memory_map_t*)tag;
            break;
        }
        // Move to next tag (8-byte aligned)
        uintptr_t next = ((uintptr_t)tag + tag->size + 7) & ~7ULL;
        tag = (multiboot_tag_t*)next;
    }

    if (mmap_tag == NULL) {
        printf("ERROR: No memory map found in multiboot info\n");
        return;
    }

    // Save memory map entries before we potentially overwrite them
    uint32_t entries_count = (mmap_tag->size - 16) / mmap_tag->entry_size;
    printf("Memory map entries: %u\n", entries_count);

    if (entries_count > MAX_MMAP_ENTRIES) {
        printf("WARNING: Too many memory map entries, truncating to %d\n", MAX_MMAP_ENTRIES);
        entries_count = MAX_MMAP_ENTRIES;
    }

    saved_mmap_count = entries_count;
    for (uint32_t i = 0; i < entries_count; i++) {
        multiboot_tag_memory_map_entry_t* entry = &mmap_tag->entries[i];
        saved_mmap[i].base_low = entry->base_addr_low;
        saved_mmap[i].base_high = entry->base_addr_high;
        saved_mmap[i].length_low = entry->length_low;
        saved_mmap[i].length_high = entry->length_high;
        saved_mmap[i].type = entry->type;
    }

    // Find the maximum usable memory address
    pmm_max_memory = 0;
    for (uint32_t i = 0; i < entries_count; i++) {
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

    // Calculate number of pages
    pmm_total_pages = pmm_max_memory / PMM_PAGE_SIZE;
    printf("Total pages: %u\n", pmm_total_pages);

    // Calculate bitmap size (in bytes, then convert to uint32_t elements)
    uint32_t bitmap_bytes = (pmm_total_pages + 7) / 8;  // Round up to byte
    pmm_bitmap_size = (bitmap_bytes + 3) / 4;  // Convert to uint32_t count

    // Place bitmap right after the kernel in memory
    // This assumes kernel_end is page-aligned or we align it
    uint32_t kernel_end = (uint32_t)&_kernel_end;
    pmm_bitmap = (uint32_t*)pmm_align_up(kernel_end);

    printf("Bitmap location: %#x - %#x (%u bytes)\n",
           (uint32_t)pmm_bitmap,
           (uint32_t)pmm_bitmap + bitmap_bytes,
           bitmap_bytes);

    // Initialize bitmap - mark all pages as used initially
    memset(pmm_bitmap, 0xFF, bitmap_bytes);
    pmm_used_pages = pmm_total_pages;

    // Now mark available regions as free using saved memory map
    for (uint32_t i = 0; i < saved_mmap_count; i++) {
        saved_mmap_entry_t* entry = &saved_mmap[i];

        if (entry->type == 1) {  // Available RAM
            uint64_t base = ((uint64_t)entry->base_high << 32) |
                           entry->base_low;
            uint64_t length = ((uint64_t)entry->length_high << 32) |
                             entry->length_low;

            // Limit to 32-bit address space
            if (base >= 0xFFFFFFFF) {
                continue;
            }

            uint32_t start = (uint32_t)base;
            uint32_t end = (uint32_t)(base + length);
            if (end > 0xFFFFFFFF || end < start) {
                end = 0xFFFFFFFF;
            }

            // Align to page boundaries
            uint32_t start_page = pmm_addr_to_page(pmm_align_up(start));
            uint32_t end_page = pmm_addr_to_page(pmm_align_down(end));

            // Mark pages as free
            for (uint32_t page = start_page; page < end_page; page++) {
                if (page < pmm_total_pages) {
                    bitmap_clear(page);
                    pmm_used_pages--;
                }
            }
        }
    }

    // Reserve first 1MB (BIOS, VGA, etc.)
    uint32_t reserved_pages = PMM_RESERVED_AREA / PMM_PAGE_SIZE;
    for (uint32_t page = 0; page < reserved_pages; page++) {
        if (!bitmap_test(page)) {
            bitmap_set(page);
            pmm_used_pages++;
        }
    }

    // Reserve kernel memory (from 1MB to end of kernel + bitmap)
    uint32_t kernel_start = (uint32_t)&_kernel_start;
    uint32_t bitmap_end = (uint32_t)pmm_bitmap + bitmap_bytes;
    uint32_t kernel_start_page = pmm_addr_to_page(pmm_align_down(kernel_start));
    uint32_t kernel_end_page = pmm_addr_to_page(pmm_align_up(bitmap_end));

    for (uint32_t page = kernel_start_page; page <= kernel_end_page; page++) {
        if (page < pmm_total_pages && !bitmap_test(page)) {
            bitmap_set(page);
            pmm_used_pages++;
        }
    }

    printf("PMM initialized successfully\n");
    pmm_print_stats();
}

uint32_t pmm_alloc_page(void)
{
    uint32_t page = bitmap_find_free();

    if (page == (uint32_t)-1) {
        printf("PMM: Out of memory!\n");
        return 0;
    }

    bitmap_set(page);
    pmm_used_pages++;

    return pmm_page_to_addr(page);
}

uint32_t pmm_alloc_pages(size_t count)
{
    if (count == 0) {
        return 0;
    }

    uint32_t page = bitmap_find_free_pages(count);

    if (page == (uint32_t)-1) {
        printf("PMM: Cannot allocate %u contiguous pages\n", count);
        return 0;
    }

    // Mark all pages as used
    for (size_t i = 0; i < count; i++) {
        bitmap_set(page + i);
        pmm_used_pages++;
    }

    return pmm_page_to_addr(page);
}

void pmm_free_page(uint32_t physical_addr)
{
    if (!pmm_is_aligned(physical_addr)) {
        printf("PMM: Warning - attempting to free unaligned address %#x\n",
               physical_addr);
        physical_addr = pmm_align_down(physical_addr);
    }

    uint32_t page = pmm_addr_to_page(physical_addr);

    if (page >= pmm_total_pages) {
        printf("PMM: Error - attempting to free invalid page %u\n", page);
        return;
    }

    if (!bitmap_test(page)) {
        printf("PMM: Warning - attempting to free already free page %u (%#x)\n",
               page, physical_addr);
        return;
    }

    bitmap_clear(page);
    pmm_used_pages--;
}

void pmm_free_pages(uint32_t physical_addr, size_t count)
{
    if (!pmm_is_aligned(physical_addr)) {
        printf("PMM: Warning - attempting to free unaligned address %#x\n",
               physical_addr);
        physical_addr = pmm_align_down(physical_addr);
    }

    uint32_t page = pmm_addr_to_page(physical_addr);

    for (size_t i = 0; i < count; i++) {
        uint32_t current_page = page + i;

        if (current_page >= pmm_total_pages) {
            printf("PMM: Error - page %u out of range\n", current_page);
            continue;
        }

        if (!bitmap_test(current_page)) {
            printf("PMM: Warning - page %u already free\n", current_page);
            continue;
        }

        bitmap_clear(current_page);
        pmm_used_pages--;
    }
}

uint32_t pmm_get_total_memory(void)
{
    return pmm_total_pages * PMM_PAGE_SIZE;
}

uint32_t pmm_get_used_memory(void)
{
    return pmm_used_pages * PMM_PAGE_SIZE;
}

uint32_t pmm_get_free_memory(void)
{
    return (pmm_total_pages - pmm_used_pages) * PMM_PAGE_SIZE;
}

void pmm_print_stats(void)
{
    uint32_t total_kb = pmm_get_total_memory() / 1024;
    uint32_t used_kb = pmm_get_used_memory() / 1024;
    uint32_t free_kb = pmm_get_free_memory() / 1024;
    uint32_t used_percent = (pmm_used_pages * 100) / pmm_total_pages;

    printf("=== Physical Memory Statistics ===\n");
    printf("Total memory: %u KB (%u MB)\n", total_kb, total_kb / 1024);
    printf("Used memory:  %u KB (%u MB) [%u%%]\n",
           used_kb, used_kb / 1024, used_percent);
    printf("Free memory:  %u KB (%u MB)\n", free_kb, free_kb / 1024);
    printf("Page size:    %u bytes\n", PMM_PAGE_SIZE);
    printf("Total pages:  %u\n", pmm_total_pages);
    printf("Used pages:   %u\n", pmm_used_pages);
    printf("Free pages:   %u\n", pmm_total_pages - pmm_used_pages);
    printf("==================================\n");
}
