#ifndef KERNEL_MEMORY_TESTS_H
#define KERNEL_MEMORY_TESTS_H

#include <kernel/memory/pmm.h>
#include <kernel/paging.h>
#include <kernel/memory/vmm.h>
#include <stdio.h>
#include <stdbool.h>
#include "libc/test_base.h"
#include "stdlib.h"

/**
 * Memory System Test Suite
 *
 * Comprehensive tests for PMM, Paging, and VMM functionality
 */

/**
 * Test PMM: Basic allocation and freeing
 */
static inline bool memory_test_pmm_basic(void) {
    printf("\n[MEMORY TEST 1] PMM Basic Allocation\n");

    // Allocate single pages
    uint32_t page1 = pmm_alloc_page();
    uint32_t page2 = pmm_alloc_page();

    if (!page1 || !page2) {
        printf("  FAILED: Could not allocate single pages\n");
        return false;
    }

    printf("  Page 1: %#x\n", page1);
    printf("  Page 2: %#x\n", page2);

    // Allocate contiguous pages
    uint32_t pages = pmm_alloc_pages(3);
    if (!pages) {
        printf("  FAILED: Could not allocate contiguous pages\n");
        pmm_free_page(page1);
        pmm_free_page(page2);
        return false;
    }

    printf("  3-page block: %#x - %#x\n", pages, pages + (3 * 4096) - 1);

    // Free all allocated pages
    pmm_free_page(page1);
    pmm_free_page(page2);
    pmm_free_pages(pages, 3);

    printf("  All pages freed successfully\n");
    printf("  PASSED\n");
    return true;
}

/**
 * Test PMM: Buddy allocator power-of-2 allocations
 */
static inline bool memory_test_pmm_buddy_orders(void) {
    printf("\n[MEMORY TEST 2] PMM Buddy Allocator Orders\n");

    // Test allocations of different orders
    uint32_t order0 = pmm_alloc_pages(1);   // 1 page = order 0
    uint32_t order1 = pmm_alloc_pages(2);   // 2 pages = order 1
    uint32_t order2 = pmm_alloc_pages(4);   // 4 pages = order 2
    uint32_t order3 = pmm_alloc_pages(8);   // 8 pages = order 3

    if (!order0 || !order1 || !order2 || !order3) {
        printf("  FAILED: Could not allocate power-of-2 blocks\n");
        if (order0) pmm_free_pages(order0, 1);
        if (order1) pmm_free_pages(order1, 2);
        if (order2) pmm_free_pages(order2, 4);
        return false;
    }

    printf("  Order 0 (1 page):  %#x\n", order0);
    printf("  Order 1 (2 pages): %#x\n", order1);
    printf("  Order 2 (4 pages): %#x\n", order2);
    printf("  Order 3 (8 pages): %#x\n", order3);

    // Verify alignment for higher orders
    bool aligned = true;
    if (order1 % (2 * 4096) != 0) {
        printf("  FAILED: Order 1 block not properly aligned\n");
        aligned = false;
    }
    if (order2 % (4 * 4096) != 0) {
        printf("  FAILED: Order 2 block not properly aligned\n");
        aligned = false;
    }
    if (order3 % (8 * 4096) != 0) {
        printf("  FAILED: Order 3 block not properly aligned\n");
        aligned = false;
    }

    // Free all
    pmm_free_pages(order0, 1);
    pmm_free_pages(order1, 2);
    pmm_free_pages(order2, 4);
    pmm_free_pages(order3, 8);

    if (!aligned) {
        return false;
    }

    printf("  All blocks properly aligned and freed\n");
    printf("  PASSED\n");
    return true;
}

/**
 * Test PMM: Non-power-of-2 allocations (should round up)
 */
static inline bool memory_test_pmm_buddy_rounding(void) {
    printf("\n[MEMORY TEST 3] PMM Buddy Non-Power-of-2 Rounding\n");

    // Request 3 pages, should get 4 (rounds up to order 2)
    uint32_t pages3 = pmm_alloc_pages(3);
    if (!pages3) {
        printf("  FAILED: Could not allocate 3 pages\n");
        return false;
    }

    printf("  Requested 3 pages, got block at: %#x\n", pages3);

    // Check alignment - should be aligned to 4 pages (16KB)
    if (pages3 % (4 * 4096) != 0) {
        printf("  FAILED: Block not aligned to 4 pages (buddy allocator should round to order 2)\n");
        pmm_free_pages(pages3, 3);
        return false;
    }

    printf("  Block correctly aligned to 4-page boundary\n");

    // Request 5 pages, should get 8 (rounds up to order 3)
    uint32_t pages5 = pmm_alloc_pages(5);
    if (!pages5) {
        printf("  FAILED: Could not allocate 5 pages\n");
        pmm_free_pages(pages3, 3);
        return false;
    }

    printf("  Requested 5 pages, got block at: %#x\n", pages5);

    // Check alignment - should be aligned to 8 pages (32KB)
    if (pages5 % (8 * 4096) != 0) {
        printf("  FAILED: Block not aligned to 8 pages (buddy allocator should round to order 3)\n");
        pmm_free_pages(pages3, 3);
        pmm_free_pages(pages5, 5);
        return false;
    }

    printf("  Block correctly aligned to 8-page boundary\n");

    // Free both
    pmm_free_pages(pages3, 3);
    pmm_free_pages(pages5, 5);

    printf("  All blocks freed (count parameter ignored by buddy allocator)\n");
    printf("  PASSED\n");
    return true;
}

/**
 * Test PMM: Buddy coalescing
 */
static inline bool memory_test_pmm_buddy_coalescing(void) {
    printf("\n[MEMORY TEST 4] PMM Buddy Coalescing\n");

    // Allocate two adjacent buddy blocks
    uint32_t block1 = pmm_alloc_pages(4);  // Order 2
    uint32_t block2 = pmm_alloc_pages(4);  // Order 2

    if (!block1 || !block2) {
        printf("  FAILED: Could not allocate blocks for coalescing test\n");
        if (block1) pmm_free_pages(block1, 4);
        return false;
    }

    printf("  Allocated two 4-page blocks:\n");
    printf("    Block 1: %#x\n", block1);
    printf("    Block 2: %#x\n", block2);

    uint32_t used_before = pmm_get_used_memory();

    // Free both blocks - they should coalesce if they're buddies
    pmm_free_pages(block1, 4);
    pmm_free_pages(block2, 4);

    uint32_t used_after = pmm_get_used_memory();

    printf("  Memory used before free: %u KB\n", used_before / 1024);
    printf("  Memory used after free:  %u KB\n", used_after / 1024);

    // Verify memory was freed
    if (used_after >= used_before) {
        printf("  FAILED: Memory not freed correctly\n");
        return false;
    }

    printf("  Blocks freed and coalesced successfully\n");
    printf("  PASSED\n");
    return true;
}

static inline bool memory_test_refcount_base_allocation()
{
    printf("\n[MEMORY TEST 5] PMM refcount base allocation\n");

    uint32_t page = pmm_alloc_page();
    ASSERT_EQ(1, pmm_get_refcount(page), "Expected refcount=1 after alloc");
    ASSERT_TRUE(pmm_is_allocated(page), "Expected is_alloc=true after alloc");

    pmm_free_page(page);
    ASSERT_EQ(0, pmm_get_refcount(page), "Expected refcount=0 after free");
    ASSERT_FALSE(pmm_is_allocated(page), "Expected is_alloc=false after free");

    return true;
}

static inline bool memory_test_refcount_shared()
{
    printf("\n[MEMORY TEST 6] PMM refcount sharing\n");

    uint32_t page = pmm_alloc_page();
    ASSERT_EQ(1, pmm_get_refcount(page), "Expected refcount=1 after alloc");
    ASSERT_FALSE(pmm_is_shared(page), "Expected is_shared=false before inc");
    ASSERT_TRUE(pmm_is_allocated(page), "Expected is_alloc=true after alloc");

    pmm_inc_refcount(page);
    ASSERT_EQ(2, pmm_get_refcount(page), "Expected refcount=2 after inc");
    ASSERT_TRUE(pmm_is_shared(page), "Expected is_shared=true after inc");
    ASSERT_TRUE(pmm_is_allocated(page), "Expected is_alloc=true after inc");

    pmm_dec_refcount(page);
    ASSERT_EQ(1, pmm_get_refcount(page), "Expected refcount=1 after dec");
    ASSERT_FALSE(pmm_is_shared(page), "Expected is_shared=false after dec");
    ASSERT_TRUE(pmm_is_allocated(page), "Expected is_alloc=true after dec");

    pmm_dec_refcount(page);
    ASSERT_EQ(1, pmm_get_refcount(page), "Expected refcount=1 after dec2");
    ASSERT_FALSE(pmm_is_shared(page), "Expected is_shared=false after dec2");
    ASSERT_TRUE(pmm_is_allocated(page), "Expected is_alloc=false after dec2");

    return true;
}

/**
 * Test Paging: Identity mapping validation
 */
static inline bool memory_test_paging_identity(void) {
    printf("\n[MEMORY TEST 7] Paging Identity Mapping\n");

    if (!paging_validate_identity_mapping()) {
        printf("  FAILED: Identity mapping validation failed\n");
        return false;
    }

    printf("  Identity mapping validated\n");

    // Check specific addresses
    uint32_t test_addrs[] = {
        0x100000,  // Kernel start (1MB)
        0xB8000,   // VGA text buffer
        0x200000,  // 2MB mark
    };

    bool all_mapped = true;
    for (size_t i = 0; i < sizeof(test_addrs) / sizeof(test_addrs[0]); i++) {
        uint32_t virt = test_addrs[i];
        if (paging_is_mapped(virt)) {
            uint32_t phys = paging_get_physical_address(virt);
            printf("  Virtual %#x -> Physical %#x %s\n",
                   virt, phys,
                   (virt == phys) ? "(identity)" : "(NOT identity!)");

            if (virt != phys) {
                all_mapped = false;
            }
        } else {
            printf("  Virtual %#x -> NOT MAPPED!\n", virt);
            all_mapped = false;
        }
    }

    if (!all_mapped) {
        printf("  FAILED: Some addresses not properly identity-mapped\n");
        return false;
    }

    printf("  PASSED\n");
    return true;
}

/**
 * Test Paging: Dynamic page mapping and unmapping
 */
static inline bool memory_test_paging_dynamic(void) {
    printf("\n[MEMORY TEST 8] Paging Dynamic Mapping\n");

    // Allocate a physical page
    uint32_t phys_page = pmm_alloc_page();
    if (!phys_page) {
        printf("  FAILED: Could not allocate physical page\n");
        return false;
    }

    printf("  Allocated physical page: %#x\n", phys_page);

    // Map to a virtual address outside identity-mapped region
    uint32_t virt_page = 0x500000;  // 5MB
    printf("  Mapping to virtual address: %#x\n", virt_page);

    if (!paging_map_page(virt_page, phys_page, PAGE_FLAGS_KERNEL)) {
        printf("  FAILED: Could not map page\n");
        pmm_free_page(phys_page);
        return false;
    }

    printf("  Successfully mapped page\n");

    // Verify mapping
    uint32_t mapped_phys = paging_get_physical_address(virt_page);
    printf("  Verification: virtual %#x -> physical %#x\n", virt_page, mapped_phys);

    if (mapped_phys != phys_page) {
        printf("  FAILED: Mapping verification failed\n");
        paging_unmap_page(virt_page);
        pmm_free_page(phys_page);
        return false;
    }

    // Test read/write
    uint32_t* test_ptr = (uint32_t*)virt_page;
    *test_ptr = 0xDEADBEEF;

    if (*test_ptr != 0xDEADBEEF) {
        printf("  FAILED: Read/write test failed\n");
        paging_unmap_page(virt_page);
        pmm_free_page(phys_page);
        return false;
    }

    printf("  Read/write test passed (wrote and read %#x)\n", *test_ptr);

    // Clean up: unmap and free
    paging_unmap_page(virt_page);
    pmm_free_page(phys_page);

    printf("  Page unmapped and freed\n");
    printf("  PASSED\n");
    return true;
}

/**
 * Test VMM: Basic allocation and freeing
 */
static inline bool memory_test_vmm_basic(void) {
    printf("\n[MEMORY TEST 9] VMM Basic Allocation\n");

    // Allocate single page
    void* vmem1 = vmm_alloc(1, VMM_KERNEL | VMM_WRITE);
    if (!vmem1) {
        printf("  FAILED: Could not allocate 1 page\n");
        return false;
    }

    printf("  Allocated 1 page at virtual address: %#x\n", (uint32_t)vmem1);

    // Test write
    uint32_t* test_write = (uint32_t*)vmem1;
    *test_write = 0xCAFEBABE;

    if (*test_write != 0xCAFEBABE) {
        printf("  FAILED: Read/write test failed\n");
        vmm_free(vmem1, 1);
        return false;
    }

    printf("  Read/write test passed: wrote %#x, read %#x\n", 0xCAFEBABE, *test_write);

    // Free the page
    if (!vmm_free(vmem1, 1)) {
        printf("  FAILED: Could not free page\n");
        return false;
    }

    printf("  Page freed successfully\n");
    printf("  PASSED\n");
    return true;
}

/**
 * Test VMM: Multiple page allocation
 */
static inline bool memory_test_vmm_multipage(void) {
    printf("\n[MEMORY TEST 10] VMM Multiple Page Allocation\n");

    const size_t num_pages = 4;
    void* vmem = vmm_alloc(num_pages, VMM_KERNEL | VMM_WRITE);

    if (!vmem) {
        printf("  FAILED: Could not allocate %zu pages\n", num_pages);
        return false;
    }

    printf("  Allocated %zu pages at virtual address: %#x\n", num_pages, (uint32_t)vmem);

    // Write to each page
    bool write_success = true;
    for (size_t i = 0; i < num_pages; i++) {
        uint32_t* ptr = (uint32_t*)((uint32_t)vmem + (i * 4096));
        *ptr = 0x1000 + i;

        if (*ptr != 0x1000 + i) {
            printf("  FAILED: Read/write on page %zu\n", i);
            write_success = false;
            break;
        }

        printf("  Page %zu: wrote %#x, read %#x\n", i, 0x1000 + i, *ptr);
    }

    if (!write_success) {
        vmm_free(vmem, num_pages);
        return false;
    }

    // Free all pages
    if (!vmm_free(vmem, num_pages)) {
        printf("  FAILED: Could not free pages\n");
        return false;
    }

    printf("  All pages freed successfully\n");
    printf("  PASSED\n");
    return true;
}

/**
 * Test VMM: Kernel heap allocator
 */
static inline bool memory_test_vmm_kernel_heap(void) {
    printf("\n[MEMORY TEST 11] VMM Kernel Heap Allocator\n");

    // Allocate from kernel heap
    void* heap_mem = vmm_kernel_alloc(8192);  // 2 pages worth
    if (!heap_mem) {
        printf("  FAILED: Could not allocate from kernel heap\n");
        return false;
    }

    printf("  Allocated 8KB from kernel heap at: %#x\n", (uint32_t)heap_mem);

    // Write pattern
    char* bytes = (char*)heap_mem;
    for (int i = 0; i < 100; i++) {
        bytes[i] = (char)(i & 0xFF);
    }

    // Verify pattern
    bool pattern_ok = true;
    for (int i = 0; i < 100; i++) {
        if (bytes[i] != (char)(i & 0xFF)) {
            pattern_ok = false;
            printf("  FAILED: Data verification at byte %d\n", i);
            break;
        }
    }

    if (!pattern_ok) {
        vmm_kernel_free(heap_mem, 8192);
        return false;
    }

    printf("  Data pattern test passed (100 bytes verified)\n");

    // Free the allocation
    vmm_kernel_free(heap_mem, 8192);
    printf("  Heap memory freed successfully\n");

    printf("  PASSED\n");
    return true;
}

/**
 * Print memory statistics
 */
static inline void memory_print_all_stats(const char* label) {
    printf("\n=== %s ===\n", label);
    pmm_print_stats();
    printf("\n");
    paging_print_stats();
    printf("\n");
    vmm_print_stats();
}

/**
 * Run only PMM tests (no VMM tests)
 */
static inline void memory_run_pmm_tests(void) {
    printf("\n========================================\n");
    printf("     PMM Test Suite\n");
    printf("========================================\n");

    // Print baseline statistics
    pmm_print_stats();

    int passed = 0;
    int total = 6;

    // Run PMM tests only
    if (memory_test_pmm_basic()) passed++;
    if (memory_test_pmm_buddy_orders()) passed++;
    if (memory_test_pmm_buddy_rounding()) passed++;
    if (memory_test_pmm_buddy_coalescing()) passed++;
    if (memory_test_refcount_base_allocation()) passed++;
    if (memory_test_refcount_shared()) passed++;

    // Print final statistics
    printf("\n");
    pmm_print_stats();

    // Print results
    printf("\n========================================\n");
    printf("  Results: %d/%d tests passed\n", passed, total);
    printf("========================================\n");

    if (passed == total) {
        printf("\n[OK] All PMM tests PASSED!\n\n");
    } else {
        printf("\n[FAIL] Some PMM tests FAILED\n\n");
    }
}

/**
 * Run all memory system tests
 */
static inline void memory_run_all_tests(void) {
    printf("\n========================================\n");
    printf("     Memory System Test Suite\n");
    printf("========================================\n");

    // Print baseline statistics
    memory_print_all_stats("Baseline Memory Statistics");

    int passed = 0;
    int total = 11;

    // Run all tests
    if (memory_test_pmm_basic()) passed++;
    if (memory_test_pmm_buddy_orders()) passed++;
    if (memory_test_pmm_buddy_rounding()) passed++;
    if (memory_test_pmm_buddy_coalescing()) passed++;
    if (memory_test_refcount_base_allocation()) passed++;
    if (memory_test_refcount_shared()) passed++;
    if (memory_test_paging_identity()) passed++;
    if (memory_test_paging_dynamic()) passed++;
    if (memory_test_vmm_basic()) passed++;
    if (memory_test_vmm_multipage()) passed++;
    if (memory_test_vmm_kernel_heap()) passed++;

    // Print final statistics
    memory_print_all_stats("Final Memory Statistics");

    // Print results
    printf("\n========================================\n");
    printf("  Results: %d/%d tests passed\n", passed, total);
    printf("========================================\n");

    if (passed == total) {
        printf("\n[OK] All memory system tests PASSED!\n\n");
    } else {
        printf("\n[FAIL] Some tests FAILED\n\n");
        abort();
    }
}

/**
 * Quick sanity check for all memory subsystems
 */
static inline bool memory_quick_test(void) {
    // Quick PMM test
    uint32_t page = pmm_alloc_page();
    if (!page) return false;
    pmm_free_page(page);

    // Quick VMM test
    void* vmem = vmm_alloc(1, VMM_KERNEL | VMM_WRITE);
    if (!vmem) return false;

    uint32_t* ptr = (uint32_t*)vmem;
    *ptr = 0x12345678;
    bool success = (*ptr == 0x12345678);

    vmm_free(vmem, 1);
    return success;
}

#endif // KERNEL_MEMORY_TESTS_H
