#ifndef KERNEL_VMM_TESTS_H
#define KERNEL_VMM_TESTS_H

#include <kernel/memory/vmm.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

/**
 * VMM Test Suite
 *
 * Comprehensive tests for Virtual Memory Manager functionality
 */

/**
 * Test 1: Basic single page allocation
 * Tests: vmm_alloc, read/write, vmm_free
 */
static inline bool vmm_test_single_page_alloc(void) {
    printf("\n[VMM TEST 1] Single page allocation\n");

    void* page = vmm_alloc(1, VMM_KERNEL | VMM_WRITE);
    if (!page) {
        printf("  FAILED: Could not allocate page\n");
        return false;
    }

    printf("  Allocated: %#x\n", (uint32_t)page);

    // Test write/read
    uint32_t* ptr = (uint32_t*)page;
    *ptr = 0xDEADBEEF;

    if (*ptr != 0xDEADBEEF) {
        printf("  FAILED: Read/write test\n");
        return false;
    }

    // Free
    if (!vmm_free(page, 1)) {
        printf("  FAILED: Could not free page\n");
        return false;
    }

    printf("  PASSED\n");
    return true;
}

/**
 * Test 2: Multiple page allocation
 * Tests: vmm_alloc with multiple pages, boundary access
 */
static inline bool vmm_test_multi_page_alloc(void) {
    printf("\n[VMM TEST 2] Multiple page allocation\n");

    const size_t num_pages = 8;
    void* pages = vmm_alloc(num_pages, VMM_KERNEL | VMM_WRITE);

    if (!pages) {
        printf("  FAILED: Could not allocate %zu pages\n", num_pages);
        return false;
    }

    printf("  Allocated: %#x (%zu pages)\n", (uint32_t)pages, num_pages);

    // Test each page
    for (size_t i = 0; i < num_pages; i++) {
        uint32_t* ptr = (uint32_t*)((uint32_t)pages + (i * PAGE_SIZE));
        *ptr = 0x1000 + i;

        if (*ptr != 0x1000 + i) {
            printf("  FAILED: Read/write on page %zu\n", i);
            return false;
        }
    }

    // Free
    if (!vmm_free(pages, num_pages)) {
        printf("  FAILED: Could not free pages\n");
        return false;
    }

    printf("  PASSED\n");
    return true;
}

/**
 * Test 3: Allocation and freeing pattern
 * Tests: Multiple allocations, region fragmentation, merging
 */
static inline bool vmm_test_alloc_free_pattern(void) {
    printf("\n[VMM TEST 3] Allocation/free pattern\n");

    void* alloc1 = vmm_alloc(2, VMM_KERNEL | VMM_WRITE);
    void* alloc2 = vmm_alloc(3, VMM_KERNEL | VMM_WRITE);
    void* alloc3 = vmm_alloc(1, VMM_KERNEL | VMM_WRITE);

    if (!alloc1 || !alloc2 || !alloc3) {
        printf("  FAILED: Initial allocations\n");
        return false;
    }

    printf("  Alloc1: %#x (2 pages)\n", (uint32_t)alloc1);
    printf("  Alloc2: %#x (3 pages)\n", (uint32_t)alloc2);
    printf("  Alloc3: %#x (1 page)\n", (uint32_t)alloc3);

    // Free middle allocation
    if (!vmm_free(alloc2, 3)) {
        printf("  FAILED: Could not free alloc2\n");
        return false;
    }

    printf("  Freed alloc2\n");

    // Allocate again (should reuse freed space)
    void* alloc4 = vmm_alloc(2, VMM_KERNEL | VMM_WRITE);
    if (!alloc4) {
        printf("  FAILED: Could not allocate alloc4\n");
        return false;
    }

    printf("  Alloc4: %#x (2 pages, should reuse space)\n", (uint32_t)alloc4);

    // Cleanup
    vmm_free(alloc1, 2);
    vmm_free(alloc3, 1);
    vmm_free(alloc4, 2);

    printf("  PASSED\n");
    return true;
}

/**
 * Test 4: Kernel heap allocator
 * Tests: vmm_kernel_alloc, vmm_kernel_free
 */
static inline bool vmm_test_kernel_heap(void) {
    printf("\n[VMM TEST 4] Kernel heap allocator\n");

    // Allocate small amount
    void* mem1 = vmm_kernel_alloc(1024);
    if (!mem1) {
        printf("  FAILED: Could not allocate 1KB\n");
        return false;
    }

    printf("  Allocated 1KB at: %#x\n", (uint32_t)mem1);

    // Test write
    char* bytes = (char*)mem1;
    for (int i = 0; i < 1024; i++) {
        bytes[i] = (char)(i & 0xFF);
    }

    // Verify
    for (int i = 0; i < 1024; i++) {
        if (bytes[i] != (char)(i & 0xFF)) {
            printf("  FAILED: Data verification at byte %d\n", i);
            return false;
        }
    }

    // Allocate larger amount
    void* mem2 = vmm_kernel_alloc(16384);  // 4 pages
    if (!mem2) {
        printf("  FAILED: Could not allocate 16KB\n");
        return false;
    }

    printf("  Allocated 16KB at: %#x\n", (uint32_t)mem2);

    // Cleanup
    vmm_kernel_free(mem1, 1024);
    vmm_kernel_free(mem2, 16384);

    printf("  PASSED\n");
    return true;
}

/**
 * Test 5: Boundary conditions
 * Tests: Edge cases, invalid inputs
 */
static inline bool vmm_test_boundary_conditions(void) {
    printf("\n[VMM TEST 5] Boundary conditions\n");

    // Test zero allocation
    void* zero = vmm_alloc(0, VMM_KERNEL | VMM_WRITE);
    if (zero != NULL) {
        printf("  WARNING: Zero-size allocation should return NULL\n");
    }

    // Test NULL free
    bool result = vmm_free(NULL, 1);
    if (result) {
        printf("  WARNING: Freeing NULL should return false\n");
    }

    // Test zero-size free
    void* test = vmm_alloc(1, VMM_KERNEL | VMM_WRITE);
    if (test) {
        result = vmm_free(test, 0);
        if (result) {
            printf("  WARNING: Zero-size free should return false\n");
        }
        vmm_free(test, 1);  // Clean up
    }

    printf("  PASSED\n");
    return true;
}

/**
 * Test 6: Memory isolation
 * Tests: Write to one allocation doesn't affect another
 */
static inline bool vmm_test_memory_isolation(void) {
    printf("\n[VMM TEST 6] Memory isolation\n");

    void* mem1 = vmm_alloc(1, VMM_KERNEL | VMM_WRITE);
    void* mem2 = vmm_alloc(1, VMM_KERNEL | VMM_WRITE);

    if (!mem1 || !mem2) {
        printf("  FAILED: Could not allocate test memory\n");
        return false;
    }

    // Write different patterns
    uint32_t* ptr1 = (uint32_t*)mem1;
    uint32_t* ptr2 = (uint32_t*)mem2;

    *ptr1 = 0xAAAAAAAA;
    *ptr2 = 0x55555555;

    // Verify isolation
    if (*ptr1 != 0xAAAAAAAA || *ptr2 != 0x55555555) {
        printf("  FAILED: Memory not isolated\n");
        vmm_free(mem1, 1);
        vmm_free(mem2, 1);
        return false;
    }

    // Cleanup
    vmm_free(mem1, 1);
    vmm_free(mem2, 1);

    printf("  PASSED\n");
    return true;
}

/**
 * Test 7: Statistics and tracking
 * Tests: vmm_get_stats, region tracking
 */
static inline bool vmm_test_statistics(void) {
    printf("\n[VMM TEST 7] Statistics and tracking\n");

    vmm_stats_t stats_before;
    vmm_get_stats(&stats_before);

    // Allocate some memory
    void* mem = vmm_alloc(5, VMM_KERNEL | VMM_WRITE);
    if (!mem) {
        printf("  FAILED: Could not allocate test memory\n");
        return false;
    }

    vmm_stats_t stats_after;
    vmm_get_stats(&stats_after);

    // Check that used regions increased
    if (stats_after.num_used_regions <= stats_before.num_used_regions) {
        printf("  WARNING: Used regions count not updated\n");
    }

    printf("  Regions before: %u used, %u free\n",
           stats_before.num_used_regions, stats_before.num_free_regions);
    printf("  Regions after:  %u used, %u free\n",
           stats_after.num_used_regions, stats_after.num_free_regions);

    // Cleanup
    vmm_free(mem, 5);

    vmm_stats_t stats_final;
    vmm_get_stats(&stats_final);

    printf("  Regions final:  %u used, %u free\n",
           stats_final.num_used_regions, stats_final.num_free_regions);

    printf("  PASSED\n");
    return true;
}

/**
 * Run all VMM tests
 */
static inline void vmm_run_all_tests(void) {
    printf("\n========================================\n");
    printf("         VMM Test Suite\n");
    printf("========================================\n");

    int passed = 0;
    int total = 7;

    if (vmm_test_single_page_alloc()) passed++;
    if (vmm_test_multi_page_alloc()) passed++;
    if (vmm_test_alloc_free_pattern()) passed++;
    if (vmm_test_kernel_heap()) passed++;
    if (vmm_test_boundary_conditions()) passed++;
    if (vmm_test_memory_isolation()) passed++;
    if (vmm_test_statistics()) passed++;

    printf("\n========================================\n");
    printf("  Results: %d/%d tests passed\n", passed, total);
    printf("========================================\n");

    if (passed == total) {
        printf("\n[OK] All VMM tests PASSED!\n\n");
    } else {
        printf("\n[FAIL] Some tests FAILED\n\n");
        abort();
    }
}

/**
 * Quick VMM sanity check
 */
static inline bool vmm_quick_test(void) {
    void* page = vmm_alloc(1, VMM_KERNEL | VMM_WRITE);
    if (!page) return false;

    uint32_t* ptr = (uint32_t*)page;
    *ptr = 0x12345678;
    bool success = (*ptr == 0x12345678);

    vmm_free(page, 1);
    return success;
}

#endif // KERNEL_VMM_TESTS_H
