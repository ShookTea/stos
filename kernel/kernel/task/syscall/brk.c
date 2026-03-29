#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../syscall.h"
#include "kernel/memory/pmm.h"
#include "kernel/memory/vmm.h"
#include "kernel/paging.h"
#include "kernel/task/scheduler.h"
#include "kernel/task/task.h"

/**
 * Sets a new program break (end of heap).
 * Behavior:
 * - If addr = NULL - return current heap_end
 * - If addr < heap_start - error (trying to shrink below heap start)
 * - If addr > heap_max - error (trying to exceed maximum heap size)
 * - If addr == heap_end - no op, return current heap_end
 * - If addr > heap_end - expand heap, return new heap_end
 * - If addr < heap_end - shrink heap, return new heap_end
 */
uint32_t sys_brk(uint32_t addr)
{
    task_t* task = scheduler_get_current_task();
    if (task == NULL) {
        return 0;
    }

    if (addr == 0) {
        return task->heap_end;
    }

    if (addr < task->heap_start) {
        printf(
            "BRK: addr %#x < heap_start %#x - PID [%u]\n",
            addr,
            task->heap_start,
            task->pid
        );
        return 0;
    }

    if (addr > task->heap_max) {
        printf(
            "BRK: addr %#x > heap_max %#x - PID [%u]\n",
            addr,
            task->heap_max,
            task->pid
        );
        return SYSCALL_ERROR;
    }

    uint32_t old_heap_end = PAGE_ALIGN_UP(task->heap_end);
    uint32_t new_heap_end = PAGE_ALIGN_UP(addr);

    if (old_heap_end == new_heap_end) {
        // no change needed
        return task->heap_end;
    }

    if (new_heap_end > old_heap_end) {
        // Expanding heap - allocate and map new pages
        uint32_t num_pages = (new_heap_end - old_heap_end) / PAGE_SIZE;
        for (uint32_t i = 0; i < num_pages; i++) {
            uint32_t vaddr = old_heap_end + (i * PAGE_SIZE);
            uint32_t phys = pmm_alloc_page();
            if (phys == 0) {
                puts("BRK: failed to allocate physical page");
                // Rollback
                for (uint32_t j = 0; j < i; j++) {
                    uint32_t vaddr_rollback = old_heap_end + (j * PAGE_SIZE);
                    uint32_t phys_rollback =
                        paging_get_physical_address(vaddr_rollback);
                    paging_unmap_page(vaddr_rollback);
                    pmm_free_page(phys_rollback);
                }
                return task->heap_end;
            }

            // Map the page as user-accessible and writeable
            if (!paging_map_page(vaddr, phys, PAGE_FLAGS_USER)) {
                printf("BRK: Failed to map page at %#x\n", vaddr);
                pmm_free_page(phys);
                // Rollback
                for (uint32_t j = 0; j < i; j++) {
                    uint32_t vaddr_rollback = old_heap_end + (j * PAGE_SIZE);
                    uint32_t phys_rollback =
                        paging_get_physical_address(vaddr_rollback);
                    paging_unmap_page(vaddr_rollback);
                    pmm_free_page(phys_rollback);
                }
                return task->heap_end;
            }

            // zero out the new page for security
            memset((void*)vaddr, 0, PAGE_SIZE);
        }
    } else {
        // Shrinking heap - unmap and free pages
        uint32_t num_pages = (old_heap_end - new_heap_end) / PAGE_SIZE;
        for (uint32_t i = 0; i < num_pages; i++) {
            uint32_t vaddr = new_heap_end + (i * PAGE_SIZE);
            uint32_t phys = paging_get_physical_address(vaddr);
            if (phys != 0) {
                paging_unmap_page(vaddr);
                pmm_free_page(phys);
            }
        }
    }

    task->heap_end = addr;
    return task->heap_end;
}
