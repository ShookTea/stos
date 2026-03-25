#include "../spinlock.h"
#include "stdlib.h"
#include <kernel/memory/pmm.h>
#include <kernel/multiboot2.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

static spinlock_t pmm_lock = SPINLOCK_INIT;

// External symbols from linker script
extern uint32_t _kernel_start;
extern uint32_t _kernel_end;

// Track if paging is enabled (set to true after pmm_init completes)
static bool pmm_paging_enabled = false;

// ============================================================================
// Buddy Allocator Configuration
// ============================================================================

#define BUDDY_MAX_ORDER 10  // Max block size: 2^10 pages = 4MB

// ============================================================================
// Buddy Allocator Data Structures
// ============================================================================

/**
 * Free block header - embedded in free physical pages
 * Stores physical addresses (not pointers) to be independent of virtual addressing
 */
typedef struct buddy_block {
    uint32_t next;  // Physical address of next block (0 if none)
    uint32_t prev;  // Physical address of prev block (0 if none)
} buddy_block_t;

/**
 * Free lists - one per order (0 to BUDDY_MAX_ORDER)
 * Order 0 = 4KB (1 page)
 * Order 1 = 8KB (2 pages)
 * Order 2 = 16KB (4 pages)
 * ...
 * Order 10 = 4MB (1024 pages)
 */
static uint32_t buddy_free_lists[BUDDY_MAX_ORDER + 1];  // Physical addresses

/**
 * Metadata storage using bitmaps
 *
 * Order map: 4 bits per page to store block order (0-10, need 4 bits)
 * Status map: 1 bit per page (0 = free, 1 = allocated)
 *
 * For a page at index P:
 * - Order: stored in order_map at bit position P*4 to P*4+3
 * - Status: stored in status_map at bit position P
 */
static uint32_t* buddy_order_map = NULL;   // 4 bits per page
static uint32_t* buddy_status_map = NULL;  // 1 bit per page

/**
 * Reference count map - 16 bits per page. Tracks how many processes/page
 * tables reference each physical page. Used for COW implementation.
 * - if 0 - page is free (should match buddy_status_map)
 * - if 1 - page is privately owned
 * - if 2 or more - page is shared between multiple processes
 */
static uint16_t* buddy_refcount_map = NULL;

static uint32_t pmm_total_pages = 0;
static uint32_t pmm_used_pages = 0;

// Track allocations per order for statistics
static uint32_t buddy_alloc_count[BUDDY_MAX_ORDER + 1];

// ============================================================================
// Physical Memory Access Helper
// ============================================================================

/**
 * Access physical memory (converts to virtual address internally)
 * This is needed to read/write free block headers
 */
static inline buddy_block_t* pmm_access_block(uint32_t phys_addr)
{
    if (pmm_paging_enabled) {
        // After paging: use PHYS_MAP_BASE region
        return (buddy_block_t*)(phys_addr + 0xC0000000);
    } else {
        // Before paging: identity mapped
        return (buddy_block_t*)phys_addr;
    }
}

// ============================================================================
// Metadata Accessors
// ============================================================================

/**
 * Get the order of a block starting at page index
 */
static inline uint8_t buddy_get_order(uint32_t page)
{
    // Each page uses 4 bits in the order map
    // Calculate which uint32_t contains our bits
    uint32_t map_index = (page * 4) / 32;
    uint32_t bit_offset = (page * 4) % 32;

    // Extract 4 bits
    uint32_t value = (buddy_order_map[map_index] >> bit_offset) & 0xF;
    return (uint8_t)value;
}

/**
 * Set the order of a block starting at page index
 */
static inline void buddy_set_order(uint32_t page, uint8_t order)
{
    uint32_t map_index = (page * 4) / 32;
    uint32_t bit_offset = (page * 4) % 32;

    // Clear the 4 bits
    buddy_order_map[map_index] &= ~(0xF << bit_offset);

    // Set the new order
    buddy_order_map[map_index] |= ((uint32_t)order << bit_offset);
}

/**
 * Check if a block is allocated
 */
static inline bool buddy_is_allocated(uint32_t page)
{
    uint32_t index = page / 32;
    uint32_t bit = page % 32;
    return (buddy_status_map[index] & (1 << bit)) != 0;
}

/**
 * Mark a block as allocated
 */
static inline void buddy_mark_allocated(uint32_t page)
{
    uint32_t index = page / 32;
    uint32_t bit = page % 32;
    buddy_status_map[index] |= (1 << bit);
}

/**
 * Mark a block as free
 */
static inline void buddy_mark_free(uint32_t page)
{
    uint32_t index = page / 32;
    uint32_t bit = page % 32;
    buddy_status_map[index] &= ~(1 << bit);
}

static inline uint16_t buddy_get_refcount(uint32_t page)
{
    if (page >= pmm_total_pages) {
        return 0;
    }
    return buddy_refcount_map[page];
}

static inline void buddy_set_refcount(uint32_t page, uint16_t count)
{
    if (page < pmm_total_pages) {
        buddy_refcount_map[page] = count;
    }
}

static inline uint16_t buddy_inc_refcount(uint32_t page)
{
    if (page >= pmm_total_pages) {
        return 0;
    }
    uint16_t current = buddy_refcount_map[page];
    if (current == UINT16_MAX) {
        // If at MAX, treat as permanently shared
        return UINT16_MAX;
    }
    buddy_refcount_map[page] = current + 1;
    return current + 1;
}

static inline uint16_t buddy_dec_refcount(uint32_t page)
{
    if (page >= pmm_total_pages) {
        return 0;
    }
    uint16_t current = buddy_refcount_map[page];
    if (current == 0) {
        printf(
            "ERR: attemted to decrement refcount for free page %u\n", page
        );
        abort(); // TODO: is abort needed here?
        return 0;
    }
    if (current == UINT16_MAX) {
        // If at MAX, treat as permanently shared
        return UINT16_MAX;
    }
    buddy_refcount_map[page] = current - 1;
    return current - 1;
}

static inline uint16_t buddy_is_page_shared(uint32_t page)
{
    if (page >= pmm_total_pages) {
        return 0;
    }
    return buddy_refcount_map[page] >= 2;
}

// ============================================================================
// Buddy Algorithm Helper Functions
// ============================================================================

/**
 * Calculate the buddy's page index
 */
static inline uint32_t buddy_get_buddy_page(uint32_t page, uint8_t order)
{
    uint32_t pages_in_block = 1 << order;
    return page ^ pages_in_block;
}

/**
 * Check if an address is properly aligned for a given order
 */
static inline bool buddy_is_order_aligned(uint32_t addr, uint8_t order)
{
    uint32_t block_size = PMM_PAGE_SIZE << order;
    return (addr & (block_size - 1)) == 0;
}

/**
 * Calculate the order needed for a given number of pages
 * Rounds up to the next power of 2
 */
static inline uint8_t buddy_calculate_order(size_t count)
{
    if (count == 0) return 0;
    if (count == 1) return 0;

    // Find the position of the highest set bit
    uint8_t order = 0;
    size_t power = 1;

    while (power < count) {
        power <<= 1;
        order++;
    }

    if (order > BUDDY_MAX_ORDER) {
        return BUDDY_MAX_ORDER;
    }

    return order;
}

// ============================================================================
// Free List Management
// ============================================================================

/**
 * Add a block to the appropriate free list
 */
static void buddy_add_to_free_list(uint32_t addr, uint8_t order)
{
    buddy_block_t* block = pmm_access_block(addr);

    // Add to head of free list (store physical addresses)
    block->next = buddy_free_lists[order];
    block->prev = 0;

    if (buddy_free_lists[order] != 0) {
        buddy_block_t* old_head = pmm_access_block(buddy_free_lists[order]);
        old_head->prev = addr;
    }

    buddy_free_lists[order] = addr;
}

/**
 * Remove a block from its free list (given physical address)
 */
static void buddy_remove_from_free_list(uint32_t addr, uint8_t order)
{
    buddy_block_t* block = pmm_access_block(addr);

    if (block->prev != 0) {
        buddy_block_t* prev_block = pmm_access_block(block->prev);
        prev_block->next = block->next;
    } else {
        // This was the head of the list
        buddy_free_lists[order] = block->next;
    }

    if (block->next != 0) {
        buddy_block_t* next_block = pmm_access_block(block->next);
        next_block->prev = block->prev;
    }
}

// ============================================================================
// Core Buddy Allocator Operations
// ============================================================================

/**
 * Split a block from higher order down to target order
 * Adds buddy blocks to appropriate free lists
 */
static void buddy_split_block(uint32_t addr, uint8_t from_order, uint8_t to_order)
{
    while (from_order > to_order) {
        from_order--;

        // Calculate buddy address (second half of split)
        uint32_t block_size = PMM_PAGE_SIZE << from_order;
        uint32_t buddy_addr = addr + block_size;

        // Add buddy to free list
        buddy_add_to_free_list(buddy_addr, from_order);

        // Update buddy's metadata
        uint32_t buddy_page = pmm_addr_to_page(buddy_addr);
        buddy_set_order(buddy_page, from_order);
        buddy_mark_free(buddy_page);
    }
}

/**
 * Coalesce a block with its buddy if possible
 * Returns the address of the coalesced block
 */
static uint32_t buddy_coalesce(uint32_t addr, uint8_t order)
{
    uint32_t page = pmm_addr_to_page(addr);

    while (order < BUDDY_MAX_ORDER) {
        // Find buddy
        uint32_t buddy_page = buddy_get_buddy_page(page, order);

        // Check if buddy exists and is valid
        if (buddy_page >= pmm_total_pages) {
            break;
        }

        // Check if buddy is free and same order
        if (buddy_is_allocated(buddy_page) || buddy_get_order(buddy_page) != order) {
            break;
        }

        // Remove buddy from free list
        uint32_t buddy_addr = pmm_page_to_addr(buddy_page);
        buddy_remove_from_free_list(buddy_addr, order);

        // Merge: use the lower address as the merged block
        if (buddy_page < page) {
            page = buddy_page;
            addr = buddy_addr;
        }

        // Increase order
        order++;
        buddy_set_order(page, order);
    }

    return addr;
}

/**
 * Allocate a block of specific order
 */
static uint32_t buddy_alloc_order(uint8_t order)
{
    if (order > BUDDY_MAX_ORDER) {
        printf("PMM: Order %u exceeds maximum %u\n", order, BUDDY_MAX_ORDER);
        return 0;
    }

    // Find a free block of sufficient size
    uint8_t current_order = order;
    while (current_order <= BUDDY_MAX_ORDER && buddy_free_lists[current_order] == 0) {
        current_order++;
    }

    // No block found
    if (current_order > BUDDY_MAX_ORDER) {
        printf("PMM: Out of memory for order %u (no free blocks)\n", order);
        for (int i = 0; i <= BUDDY_MAX_ORDER; i++) {
            if (buddy_free_lists[i] != 0) {
                printf("  Order %d has free blocks\n", i);
            }
        }
        return 0;
    }

    // Remove block from free list
    uint32_t addr = buddy_free_lists[current_order];
    buddy_remove_from_free_list(addr, current_order);

    uint32_t page = pmm_addr_to_page(addr);

    // Split down to requested order if necessary
    if (current_order > order) {
        buddy_split_block(addr, current_order, order);
    }

    // Mark as allocated
    buddy_set_order(page, order);
    buddy_mark_allocated(page);

    uint32_t pages_in_block = 1 << order;
    // Initialize ref count to 1 for all pages in the block.
    for (uint32_t i = 0; i < pages_in_block; i++) {
        buddy_set_refcount(page + i, 1);
    }
    // Update statistics
    pmm_used_pages += pages_in_block;
    buddy_alloc_count[order]++;

    return addr;
}

/**
 * Free a block and coalesce with buddies
 */
static void buddy_free_order(uint32_t addr, uint8_t order)
{
    if (!pmm_is_aligned(addr)) {
        printf("PMM: Warning - freeing unaligned address %#x\n", addr);
        addr = pmm_align_down(addr);
    }

    uint32_t page = pmm_addr_to_page(addr);

    if (page >= pmm_total_pages) {
        printf("PMM: Error - invalid page %u\n", page);
        return;
    }

    if (!buddy_is_allocated(page)) {
        printf("PMM: Warning - freeing already free block at %#x\n", addr);
        return;
    }

    uint32_t pages_in_block = 1 << order;

    // Check if any page in the block still has references
    for (uint32_t i = 0; i < pages_in_block; i++) {
        uint16_t refcount = buddy_get_refcount(page + i);
        if (refcount > 1) {
            printf(
                "ERR: cannot free block at %#x (order %u): ",
                addr,
                order
            );
            printf(
                "page %u still has %u references\n",
                page + i,
                refcount
            );
            return;
        }
    }

    // Mark as free
    buddy_mark_free(page);

    // Clear refcount for all pages in the block
    for (uint32_t i = 0; i < pages_in_block; i++) {
        buddy_set_refcount(page + i, 0);
    }
    // Update statistics
    pmm_used_pages -= pages_in_block;

    // Decrement allocation count (with underflow protection)
    if (buddy_alloc_count[order] > 0) {
        buddy_alloc_count[order]--;
    } else {
        printf("PMM: Warning - tried to decrement buddy_alloc_count[%u] when already 0\n", order);
    }

    // Coalesce with buddies
    addr = buddy_coalesce(addr, order);
    page = pmm_addr_to_page(addr);
    order = buddy_get_order(page);

    // Add to free list
    buddy_add_to_free_list(addr, order);
}

// ============================================================================
// Initialization Helpers
// ============================================================================

/**
 * Add a memory region to the buddy allocator
 * Breaks the region into largest possible buddy blocks
 * This function processes available RAM regions and adds them to free lists
 */
static void buddy_add_region(uint32_t start_addr, uint32_t end_addr)
{
    // Align region to page boundaries
    start_addr = pmm_align_up(start_addr);
    end_addr = pmm_align_down(end_addr);

    if (start_addr >= end_addr) {
        return;
    }

    uint32_t current_addr = start_addr;

    while (current_addr < end_addr) {
        uint32_t remaining_size = end_addr - current_addr;
        uint32_t remaining_pages = remaining_size / PMM_PAGE_SIZE;

        if (remaining_pages == 0) {
            break;
        }

        uint32_t page = pmm_addr_to_page(current_addr);

        // Skip if this page was already marked as reserved
        if (buddy_is_allocated(page)) {
            current_addr += PMM_PAGE_SIZE;
            continue;
        }

        // Find the largest order that fits and is aligned
        uint8_t order = 0;

        for (uint8_t o = BUDDY_MAX_ORDER; o > 0; o--) {
            uint32_t pages_in_block = 1 << o;

            // Check if block fits in remaining space
            if (pages_in_block <= remaining_pages) {
                // Check if current address is properly aligned for this order
                if (buddy_is_order_aligned(current_addr, o)) {
                    // Check if all pages in this block are not reserved
                    bool all_free = true;
                    for (uint32_t p = 0; p < pages_in_block; p++) {
                        if (page + p >= pmm_total_pages || buddy_is_allocated(page + p)) {
                            all_free = false;
                            break;
                        }
                    }
                    if (all_free) {
                        order = o;
                        break;
                    }
                }
            }
        }

        // Add block to free list
        uint32_t pages_in_block = 1 << order;
        buddy_set_order(page, order);
        buddy_mark_free(page);
        buddy_add_to_free_list(current_addr, order);

        // Statistics are already correct (pages start as free)
        // No need to modify pmm_used_pages here

        // Move to next block
        current_addr += pages_in_block * PMM_PAGE_SIZE;
    }
}

/**
 * Reserve a memory region (mark as allocated)
 * Simple version: just mark pages as allocated, don't manipulate free lists
 * This should be called BEFORE buddy_add_region adds blocks to free lists
 * Note: All pages start as allocated, so this is just ensuring they stay that way
 */
static void buddy_reserve_region(uint32_t start_addr, uint32_t end_addr)
{
    start_addr = pmm_align_down(start_addr);
    end_addr = pmm_align_up(end_addr);

    if (start_addr >= end_addr) {
        return;
    }

    // Mark pages in this region as allocated (reserved)
    uint32_t count = 0;
    for (uint32_t addr = start_addr; addr < end_addr; addr += PMM_PAGE_SIZE) {
        uint32_t page = pmm_addr_to_page(addr);

        if (page >= pmm_total_pages) {
            break;
        }

        if (!buddy_is_allocated(page)) {
            buddy_mark_allocated(page);
            pmm_used_pages++;
            count++;
        }
    }
    printf("  Reserved %u pages from %#x to %#x\n", count, start_addr, end_addr);
}

// ============================================================================
// PMM Public API Implementation
// ============================================================================

void pmm_init()
{
    // Get memory information from multiboot2
    uint32_t pmm_max_memory = multiboot2_get_max_memory();

    if (pmm_max_memory == 0) {
        // No usable memory found
        return;
    }

    // Calculate total pages
    pmm_total_pages = pmm_max_memory / PMM_PAGE_SIZE;

    // Calculate sizes for metadata
    // 4 bits per page
    size_t order_map_size = (pmm_total_pages * 4 + 7) / 8;
    // 1 bit per page
    size_t status_map_size = (pmm_total_pages + 7) / 8;
    // 16 bits per page
    size_t refcount_map_size = pmm_total_pages * sizeof(uint16_t);

    order_map_size = pmm_align_up(order_map_size);
    status_map_size = pmm_align_up(status_map_size);
    refcount_map_size = pmm_align_up(refcount_map_size);

    // Place metadata after kernel and all MB2 modules
    uint32_t kernel_end = (uint32_t)&_kernel_end;
    uint32_t safe_metadata_start = kernel_end;
    for (uint32_t i = 0; i < multiboot2_get_modules_count(); i++) {
        multiboot_tag_boot_module_t* tag = multiboot2_get_boot_module_entry(i);
        if (tag->module_phys_addr_end > safe_metadata_start) {
            safe_metadata_start = tag->module_phys_addr_end;
        }
    }
    uint32_t metadata_start = pmm_align_up(safe_metadata_start);

    buddy_order_map = (uint32_t*)metadata_start;
    buddy_status_map = (uint32_t*)((uint8_t*)buddy_order_map + order_map_size);
    buddy_refcount_map = (uint16_t*)(
        (uint8_t*)buddy_status_map + status_map_size
    );

    uint32_t metadata_end = (uint32_t)buddy_refcount_map + refcount_map_size;

    // Initialize metadata - mark all as FREE initially
    // We'll mark reserved regions as allocated afterwards
    memset(buddy_order_map, 0, order_map_size);
    memset(buddy_status_map, 0, status_map_size);
    memset(buddy_refcount_map, 0, refcount_map_size);

    // Initialize free lists
    for (int i = 0; i <= BUDDY_MAX_ORDER; i++) {
        buddy_free_lists[i] = 0;  // 0 = empty list
        buddy_alloc_count[i] = 0;
    }

    pmm_used_pages = 0;  // Start with all free, will increment as we reserve

    // First, mark reserved regions as allocated BEFORE adding free regions
    // 1. Reserve first 1MB (BIOS, VGA, etc.)
    buddy_reserve_region(0, PMM_RESERVED_AREA);

    // 2. Reserve kernel and metadata
    uint32_t kernel_start = (uint32_t)&_kernel_start;
    buddy_reserve_region(kernel_start, metadata_end);

    // 3. Reserve MB2 modules data
    for (uint32_t i = 0; i < multiboot2_get_modules_count(); i++) {
        multiboot_tag_boot_module_t* tag = multiboot2_get_boot_module_entry(i);
        buddy_reserve_region(
            tag->module_phys_addr_start,
            tag->module_phys_addr_end
        );
    }

    // Now process multiboot2 memory map and add available regions
    // The buddy_add_region function will skip pages already marked as allocated
    uint32_t mmap_count = multiboot2_get_mmap_count();

    for (uint32_t i = 0; i < mmap_count; i++) {
        const saved_mmap_entry_t* entry = multiboot2_get_mmap_entry(i);
        if (entry == NULL) {
            break;
        }

        // Only process available RAM (type 1)
        if (entry->type != 1) {
            continue;
        }

        uint64_t base = ((uint64_t)entry->base_high << 32) | entry->base_low;
        uint64_t length = ((uint64_t)entry->length_high << 32) | entry->length_low;

        // Limit to 32-bit address space
        if (base >= 0xFFFFFFFF) {
            continue;
        }

        uint32_t start = (uint32_t)base;
        uint32_t end = (uint32_t)(base + length);
        if (end > 0xFFFFFFFF || end < start) {
            end = 0xFFFFFFFF;
        }

        // Add this region to buddy allocator (will skip reserved pages)
        buddy_add_region(start, end);
    }
}

void pmm_set_paging_enabled(bool enabled)
{
    pmm_paging_enabled = enabled;
}

uint32_t pmm_alloc_page(void)
{
    spinlock_acquire(&pmm_lock);
    uint32_t result = buddy_alloc_order(0);  // Order 0 = 1 page
    spinlock_release(&pmm_lock);
    return result;
}

uint32_t pmm_alloc_pages(size_t count)
{
    if (count == 0) {
        return 0;
    }

    spinlock_acquire(&pmm_lock);
    uint8_t order = buddy_calculate_order(count);
    uint32_t result = buddy_alloc_order(order);
    spinlock_release(&pmm_lock);
    return result;
}

void pmm_free_page(uint32_t addr)
{
    spinlock_acquire(&pmm_lock);
    uint32_t page = pmm_addr_to_page(addr);
    uint8_t order = buddy_get_order(page);
    buddy_free_order(addr, order);
    spinlock_release(&pmm_lock);
}

void pmm_free_pages(uint32_t addr, size_t count __attribute__((unused)))
{
    spinlock_acquire(&pmm_lock);
    // Look up order from metadata instead of using count
    uint32_t page = pmm_addr_to_page(addr);
    uint8_t order = buddy_get_order(page);
    buddy_free_order(addr, order);
    spinlock_release(&pmm_lock);
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
    uint32_t used_percent = pmm_total_pages > 0 ? (pmm_used_pages * 100) / pmm_total_pages : 0;

    printf("=== Physical Memory Statistics ===\n");
    printf("Total memory: %u KB (%u MB)\n", total_kb, total_kb / 1024);
    printf("Used memory:  %u KB (%u MB) [%u%%]\n",
           used_kb, used_kb / 1024, used_percent);
    printf("Free memory:  %u KB (%u MB)\n", free_kb, free_kb / 1024);
    printf("Page size:    %u bytes\n", PMM_PAGE_SIZE);
    printf("Total pages:  %u\n", pmm_total_pages);
    printf("Used pages:   %u\n", pmm_used_pages);
    printf("Free pages:   %u\n", pmm_total_pages - pmm_used_pages);

    // Buddy allocator specific stats
    printf("\n--- Buddy Allocator Stats ---\n");
    printf("Max order:    %u (max block size: %u KB)\n",
           BUDDY_MAX_ORDER, (PMM_PAGE_SIZE << BUDDY_MAX_ORDER) / 1024);

    printf("\nFree blocks per order:\n");
    for (int order = 0; order <= BUDDY_MAX_ORDER; order++) {
        uint32_t count = 0;
        uint32_t block_addr = buddy_free_lists[order];
        uint32_t max_iterations = 10000;  // Safety limit

        while (block_addr != 0 && count < max_iterations) {
            // Validate address is in reasonable range
            if (block_addr >= pmm_get_total_memory()) {
                printf("  Order %2u: CORRUPTED (invalid block address %#x)\n",
                       order, block_addr);
                break;
            }

            count++;
            buddy_block_t* block = pmm_access_block(block_addr);
            block_addr = block->next;
        }

        if (count >= max_iterations) {
            printf("  Order %2u: CORRUPTED (too many blocks, possible loop)\n", order);
        } else if (count > 0 || buddy_alloc_count[order] > 0) {
            uint32_t block_size_kb = (PMM_PAGE_SIZE << order) / 1024;
            printf("  Order %2u (%4u KB): %u free, %u allocated\n",
                   order, block_size_kb, count, buddy_alloc_count[order]);
        }
    }

    printf("==================================\n");
}

// ============================================================================
// Order-based API (for advanced usage)
// ============================================================================

uint32_t pmm_alloc_order(uint8_t order)
{
    return buddy_alloc_order(order);
}

void pmm_free_order(uint32_t addr)
{
    uint32_t page = pmm_addr_to_page(addr);
    uint8_t order = buddy_get_order(page);
    buddy_free_order(addr, order);
}

uint8_t pmm_get_order(uint32_t addr)
{
    uint32_t page = pmm_addr_to_page(addr);
    return buddy_get_order(page);
}

bool pmm_is_allocated(uint32_t addr)
{
    uint32_t page = pmm_addr_to_page(addr);
    if (page >= pmm_total_pages) {
        return false;
    }
    return buddy_is_allocated(page);
}

uint16_t pmm_get_refcount(uint32_t phys_addr)
{
    spinlock_acquire(&pmm_lock);
    if (!pmm_is_aligned(phys_addr)) {
        printf("ERR: get_refcount called with unaligned addr %#x\n", phys_addr);
        spinlock_release(&pmm_lock);
        return 0;
    }

    uint32_t page = pmm_addr_to_page(phys_addr);
    uint16_t result = buddy_get_refcount(page);
    spinlock_release(&pmm_lock);
    return result;
}

uint16_t pmm_inc_refcount(uint32_t phys_addr)
{
    spinlock_acquire(&pmm_lock);
    if (!pmm_is_aligned(phys_addr)) {
        printf("ERR: inc_refcount called with unaligned addr %#x\n", phys_addr);
        spinlock_release(&pmm_lock);
        return 0;
    }

    uint32_t page = pmm_addr_to_page(phys_addr);
    if (!buddy_is_allocated(page)) {
        printf("ERR: inc_refcount called on free page %#x\n", phys_addr);
        abort(); // TODO: is abort() needed here?
        spinlock_release(&pmm_lock);
        return 0;
    }

    uint16_t result = buddy_inc_refcount(page);
    spinlock_release(&pmm_lock);
    return result;
}

uint16_t pmm_dec_refcount(uint32_t phys_addr)
{
    spinlock_acquire(&pmm_lock);
    if (!pmm_is_aligned(phys_addr)) {
        printf("ERR: dec_refcount called with unaligned addr %#x\n", phys_addr);
        spinlock_release(&pmm_lock);
        return 0;
    }

    uint32_t page = pmm_addr_to_page(phys_addr);
    if (!buddy_is_allocated(page)) {
        printf("ERR: dec_refcount called on free page %#x\n", phys_addr);
        abort(); // TODO: is abort() needed here?
        spinlock_release(&pmm_lock);
        return 0;
    }

    uint16_t result = buddy_dec_refcount(page);
    spinlock_release(&pmm_lock);
    return result;
}

bool pmm_is_shared(uint32_t phys_addr)
{
    spinlock_acquire(&pmm_lock);
    if (!pmm_is_aligned(phys_addr)) {
        printf("ERR: is_shared called with unaligned addr %#x\n", phys_addr);
        spinlock_release(&pmm_lock);
        return false;
    }

    uint32_t page = pmm_addr_to_page(phys_addr);
    bool result = buddy_is_page_shared(page);
    spinlock_release(&pmm_lock);
    return result;
}
