#ifndef KERNEL_MEMORY_LEAK_TESTS_H
#define KERNEL_MEMORY_LEAK_TESTS_H

#include "kernel/memory/kmalloc.h"
#include "kernel/memory/pmm.h"
#include "kernel/memory/slab.h"
#include "kernel/memory/vmm.h"
#include "memory_tests.h"
#include "vmm_tests.h"
#include "kmalloc_tests.h"
#include <stdio.h>
#include <stdint.h>

#define KIB 1024
#define MIB (1024 * 1024)

static inline void print_memory_stats(
    uint32_t pmm_used_memory,
    vmm_stats_t vmm_stats,
    kmalloc_stats_t kmalloc_stats,
    slab_stats_t slab_stats
) {
    printf("  [PMM] used memory: %d KiB\n", pmm_used_memory / KIB);
    printf(
        "  [VMM] used virtual space: %d MiB\n",
       vmm_stats.used_virtual_space / MIB
    );
    printf(
        "  [VMM] used regions: %d\n",
        vmm_stats.num_used_regions
    );
    printf(
        "  [VMM] kernel heap current: %#x\n",
        vmm_stats.kernel_heap_current
    );
    printf(
        "  [kmalloc] current usage: %d KiB\n",
        kmalloc_stats.current_usage / KIB
    );
    printf(
        "  [kmalloc] active allocations: %zu\n",
        (kmalloc_stats.num_small_allocations + kmalloc_stats.num_large_allocations) -
        (kmalloc_stats.num_small_frees + kmalloc_stats.num_large_frees)
    );
    printf(
        "  [slab] current usage: %d B\n",
        slab_stats.current_usage
    );
}

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

    puts("---  stats before test   ---");
    print_memory_stats(
        pmm_used_memory_start,
        vmm_stats_start,
        kmalloc_stats_start,
        slab_stats_start
    );

    puts("---   stats after test   ---");
    print_memory_stats(
        pmm_used_memory_end,
        vmm_stats_end,
        kmalloc_stats_end,
        slab_stats_end
    );

}

#endif
