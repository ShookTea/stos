#ifndef KERNEL_PAGE_FAULT_TESTS_H
#define KERNEL_PAGE_FAULT_TESTS_H

#include <stdint.h>
#include <stdio.h>
#include <kernel/paging.h>

/**
 * Page Fault Handler Test Utilities
 *
 * These functions intentionally trigger page faults to test the page fault
 * handler implementation. They should only be called when you want to test
 * the fault handling mechanism.
 *
 * WARNING: All these functions will cause the system to halt when they
 * trigger a page fault! Only use them for testing purposes.
 */

/**
 * Test 1: Access unmapped memory (page not present)
 *
 * Attempts to read from a virtual address that is not mapped to any
 * physical page. This should trigger a page fault with:
 * - Error code bit 0 = 0 (page not present)
 * - Error code bit 1 = 0 (read operation)
 * - Error code bit 2 = 0 (supervisor mode)
 */
static inline void test_page_fault_unmapped_read(void)
{
    puts("\n=== Test: Access Unmapped Memory (Read) ===");
    puts("Attempting to read from unmapped address 0x800000...");
    puts("Expected: Page fault with error code 0x0");
    puts("(Page not present, read, supervisor)\n");

    volatile uint32_t* unmapped = (uint32_t*)0x800000;  // 8MB - not mapped
    volatile uint32_t value = *unmapped;  // This will page fault
    (void)value;  // Suppress unused warning (we never get here)

    puts("ERROR: Should have page faulted but didn't!");
}

/**
 * Test 2: Write to unmapped memory (page not present)
 *
 * Attempts to write to a virtual address that is not mapped to any
 * physical page. This should trigger a page fault with:
 * - Error code bit 0 = 0 (page not present)
 * - Error code bit 1 = 1 (write operation)
 * - Error code bit 2 = 0 (supervisor mode)
 */
static inline void test_page_fault_unmapped_write(void)
{
    puts("\n=== Test: Access Unmapped Memory (Write) ===");
    puts("Attempting to write to unmapped address 0x900000...");
    puts("Expected: Page fault with error code 0x2");
    puts("(Page not present, write, supervisor)\n");

    volatile uint32_t* unmapped = (uint32_t*)0x900000;  // 9MB - not mapped
    *unmapped = 0xDEADBEEF;  // This will page fault

    puts("ERROR: Should have page faulted but didn't!");
}

/**
 * Test 3: Write to read-only page (page protection violation)
 *
 * Maps a page without write permission, then attempts to write to it.
 * This should trigger a page fault with:
 * - Error code bit 0 = 1 (page protection violation)
 * - Error code bit 1 = 1 (write operation)
 * - Error code bit 2 = 0 (supervisor mode)
 *
 * Note: This test requires the ability to map read-only pages.
 * Currently, all kernel pages are mapped as read-write, so this test
 * is commented out but provided for future use.
 */
static inline void test_page_fault_readonly_write(void)
{
    puts("\n=== Test: Write to Read-Only Page ===");
    puts("This test requires read-only page mapping support.");
    puts("Skipping for now - all kernel pages are read-write.\n");

    /* Uncomment when read-only mapping is supported:

    uint32_t phys = pmm_alloc_page();
    if (!phys) {
        puts("ERROR: Failed to allocate physical page for test");
        return;
    }

    uint32_t virt = 0xA00000;  // 10MB virtual address

    // Map as present but NOT writable (only PAGE_PRESENT flag)
    if (!paging_map_page(virt, phys, PAGE_PRESENT)) {
        puts("ERROR: Failed to map page for test");
        pmm_free_page(phys);
        return;
    }

    puts("Mapped read-only page at 0xA00000");
    puts("Attempting to write to read-only page...");
    puts("Expected: Page fault with error code 0x3");
    puts("(Page protection violation, write, supervisor)\n");

    volatile uint32_t* readonly = (uint32_t*)virt;
    *readonly = 0x12345678;  // This should page fault

    puts("ERROR: Should have page faulted but didn't!");
    */
}

/**
 * Test 4: Access null pointer (classic bug detection)
 *
 * Attempts to dereference a NULL pointer. While the first page (0x0)
 * may be identity-mapped in the current implementation, this test
 * demonstrates how the page fault handler would catch null pointer
 * dereferences if that page were unmapped.
 */
static inline void test_page_fault_null_pointer(void)
{
    puts("\n=== Test: NULL Pointer Dereference ===");
    puts("NOTE: First page (0x0) is currently identity-mapped,");
    puts("so this may not fault in the current implementation.");
    puts("Attempting to dereference NULL...\n");

    volatile uint32_t* null_ptr = (uint32_t*)0;
    volatile uint32_t value = *null_ptr;  // May or may not fault
    (void)value;

    puts("NULL pointer access completed (page is mapped)");
}

/**
 * Test 5: Access far beyond kernel space
 *
 * Attempts to access a very high virtual address that is definitely
 * not mapped. This tests the handler with addresses in different
 * page directory entries.
 */
static inline void test_page_fault_far_address(void)
{
    puts("\n=== Test: Access Far Unmapped Address ===");
    puts("Attempting to access address 0xF0000000...");
    puts("Expected: Page fault with error code 0x0");
    puts("(Page not present, read, supervisor)\n");

    volatile uint32_t* far_address = (uint32_t*)0xF0000000;  // 3.75GB
    volatile uint32_t value = *far_address;  // This will page fault
    (void)value;

    puts("ERROR: Should have page faulted but didn't!");
}

/**
 * Run all page fault tests
 *
 * WARNING: This will run all tests sequentially. The first test that
 * triggers a page fault will halt the system. Uncomment the tests
 * you want to run one at a time.
 */
static inline void run_page_fault_tests(void)
{
    puts("\n");
    puts("==============================================");
    puts("  PAGE FAULT HANDLER TEST SUITE");
    puts("==============================================");
    puts("");
    puts("WARNING: These tests intentionally cause page faults!");
    puts("The system will halt when a page fault occurs.");
    puts("Test one at a time by uncommenting individual tests.");
    puts("");

    // Uncomment ONE test at a time:

    // test_page_fault_unmapped_read();
    // test_page_fault_unmapped_write();
    // test_page_fault_readonly_write();
    // test_page_fault_null_pointer();
    // test_page_fault_far_address();

    puts("No tests are currently enabled.");
    puts("Uncomment a test in run_page_fault_tests() to run it.");
}

#endif
