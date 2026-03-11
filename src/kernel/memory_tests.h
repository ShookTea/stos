#ifndef KERNEL_MEMORY_TESTS_H
#define KERNEL_MEMORY_TESTS_H

#include <kernel/pmm.h>
#include <kernel/paging.h>
#include <kernel/vmm.h>
#include <stdio.h>
#include <stdbool.h>
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
 * Test Paging: Identity mapping validation
 */
static inline bool memory_test_paging_identity(void) {
    printf("\n[MEMORY TEST 2] Paging Identity Mapping\n");

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
    printf("\n[MEMORY TEST 3] Paging Dynamic Mapping\n");

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
    printf("\n[MEMORY TEST 4] VMM Basic Allocation\n");

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
    printf("\n[MEMORY TEST 5] VMM Multiple Page Allocation\n");

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
    printf("\n[MEMORY TEST 6] VMM Kernel Heap Allocator\n");

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
 * Run all memory system tests
 */
static inline void memory_run_all_tests(void) {
    printf("\n========================================\n");
    printf("     Memory System Test Suite\n");
    printf("========================================\n");

    // Print baseline statistics
    memory_print_all_stats("Baseline Memory Statistics");

    int passed = 0;
    int total = 6;

    // Run all tests
    if (memory_test_pmm_basic()) passed++;
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
