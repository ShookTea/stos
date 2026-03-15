#ifndef KERNEL_MEMORY_LEAK_TESTS_H
#define KERNEL_MEMORY_LEAK_TESTS_H

#include "kernel/memory/kmalloc.h"
#include "kernel/memory/pmm.h"
#include "kernel/memory/slab.h"
#include "kernel/memory/vmm.h"
#include "memory_tests.h"
#include "stdlib.h"
#include "vmm_tests.h"
#include "kmalloc_tests.h"
#include <stdio.h>
#include <stdint.h>

#define KIB 1024
#define MIB (1024 * 1024)

static inline void run_all_tests()
{
    memory_run_pmm_tests();
    vmm_run_all_tests();
    kmalloc_run_all_tests();
}

/**
 * Tests for memory leak:
 * 1. Load memory statistics
 * 2. Run all memory tests 5 times in a row
 * 3. Check memory statistics again, for any potential changes
 */
static inline void memory_leak_run_test()
{
    puts("=== memory leak test run ===");

    uint32_t pmm_used_memory_start = pmm_get_used_memory();
    vmm_stats_t vmm_stats_start;
    kmalloc_stats_t kmalloc_stats_start;
    slab_stats_t slab_stats_start;
    vmm_get_stats(&vmm_stats_start);
    kmalloc_get_stats(&kmalloc_stats_start);
    slab_get_stats(&slab_stats_start);

    puts("");
    puts("run 1");
    run_all_tests();
    puts("run 2");
    run_all_tests();
    puts("run 3");
    run_all_tests();
    puts("run 4");
    run_all_tests();
    puts("run 5");
    run_all_tests();

    uint32_t pmm_used_memory_end = pmm_get_used_memory();
    vmm_stats_t vmm_stats_end;
    kmalloc_stats_t kmalloc_stats_end;
    slab_stats_t slab_stats_end;
    vmm_get_stats(&vmm_stats_end);
    kmalloc_get_stats(&kmalloc_stats_end);
    slab_get_stats(&slab_stats_end);

    bool fail = false;
    puts("");
    puts(" === memory stats changes ===");
    printf(
        "  [PMM] used memory: %d KiB -> %d KiB\n",
        pmm_used_memory_start / KIB,
        pmm_used_memory_end / KIB
    );
    if (pmm_used_memory_start != pmm_used_memory_end) {
        fail = true;
    }

    printf(
        "  [VMM] used virtual space: %d MiB -> %d MiB\n",
       vmm_stats_start.used_virtual_space / MIB,
       vmm_stats_end.used_virtual_space / MIB
    );
    if (vmm_stats_start.used_virtual_space
        != vmm_stats_end.used_virtual_space) {
        fail = true;
    }

    printf(
        "  [VMM] used regions: %d -> %d\n",
        vmm_stats_start.num_used_regions,
        vmm_stats_end.num_used_regions
    );
    if (vmm_stats_start.num_used_regions != vmm_stats_end.num_used_regions) {
        fail = true;
    }

    printf(
        "  [VMM] kernel heap current: %#x -> %#x\n",
        vmm_stats_start.kernel_heap_current,
        vmm_stats_end.kernel_heap_current
    );
    if (vmm_stats_start.kernel_heap_current
        != vmm_stats_end.kernel_heap_current) {
        fail = true;
    }

    printf(
        "  [kmalloc] current usage: %d KiB -> %d KiB\n",
        kmalloc_stats_start.current_usage / KIB,
        kmalloc_stats_end.current_usage / KIB
    );
    if (kmalloc_stats_start.current_usage != kmalloc_stats_end.current_usage) {
        fail = true;
    }

    size_t active_allocs_start =
        (kmalloc_stats_start.num_small_allocations +
            kmalloc_stats_start.num_large_allocations) -
        (kmalloc_stats_start.num_small_frees +
            kmalloc_stats_start.num_large_frees);

    size_t active_allocs_end =
        (kmalloc_stats_end.num_small_allocations +
            kmalloc_stats_end.num_large_allocations) -
        (kmalloc_stats_end.num_small_frees +
            kmalloc_stats_end.num_large_frees);
    printf(
        "  [kmalloc] active allocations: %zu -> %zu\n",
        active_allocs_start,
        active_allocs_end
    );
    if (active_allocs_start != active_allocs_end) {
        fail = true;
    }

    printf(
        "  [slab] current usage: %d B -> %d B\n",
        slab_stats_start.current_usage,
        slab_stats_end.current_usage
    );
    if (slab_stats_start.current_usage != slab_stats_end.current_usage) {
        fail = true;
    }

    if (fail) {
        puts("memleak test FAILED!");
        abort();
    } else {
        puts("memleak test completed successfully.");
    }
}

#endif
