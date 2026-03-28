#include <kernel/memory/pmm.h>
#include <kernel/paging.h>
#include <stdlib.h>
#include <kernel/memory/kmalloc.h>
#include <kernel/memory/vmm.h>
#include <kernel/task/task.h>
#include <kernel/elf.h>
#include <stdio.h>
#include <string.h>

// Space between heap and stack (e.g. 1 MB)
#define MIN_HEAP_STACK_GAP (1 * 1024 * 1024) // 1 MB

static inline bool in_userspace(uint32_t addr)
{
    return addr < VMM_USER_END;
}

task_t* elf_create_task(const char* name, void* elf_data)
{
    elf_t* parsed = kmalloc(sizeof(elf_t));
    elf_parse(elf_data, parsed);
    if (!parsed->success) {
        return NULL;
    }

    printf("ELF: Entry point: %#x\n", parsed->entry_point);
    printf(
        "ELF: min_vaddr=%#x, max_vaddr=%#x\n",
        parsed->max_vaddr,
        parsed->max_vaddr
    );

    // Check if entrypoint is in userspace
    if (!in_userspace(parsed->entry_point)) {
        printf(
            "%s entrypoint outside of user space (%#x)\n",
            "ELF invalid:",
            parsed->entry_point
        );
        return NULL;
    }

    // Check if all segments are in userspace
    for (size_t i = 0; i < parsed->segment_count; i++) {
        uint32_t start = parsed->segments[i].vaddr;
        uint32_t end = start + parsed->segments[i].memsz;
        if (!in_userspace(start)) {
            printf(
                "%s segment %u has addr outside of user space (%#x)\n",
                "ELF invalid:",
                i,
                start
            );
            return NULL;
        }
        if (!in_userspace(end)) {
            printf(
                "%s segment %u has addr+size outside of user space (%#x)\n",
                "ELF invalid:",
                i,
                end
            );
            return NULL;
        }
    }

    task_t* task = task_create(name, (void (*)())parsed->entry_point, false);
    void* old_dir = paging_get_current_directory();
    paging_switch_directory(task->page_dir_virt);

    // Load all segments to the memory
    for (size_t i = 0; i < parsed->segment_count; i++) {
        elf_segment_t segment = parsed->segments[i];

        // Calculate page-aligned range
        uint32_t vaddr_start = PAGE_ALIGN_DOWN(segment.vaddr);
        uint32_t vaddr_end = PAGE_ALIGN_UP(segment.vaddr + segment.memsz);
        uint32_t num_pages = (vaddr_end - vaddr_start) / PAGE_SIZE;

        // Allocate and map each page
        for (uint32_t p = 0; p < num_pages; p++) {
            uint32_t vaddr = vaddr_start + (p * PAGE_SIZE);
            if (paging_is_mapped(vaddr)) {
                printf("ELF error: vaddr %#x is already mapped\n", vaddr);
                paging_switch_directory(old_dir);
                // TODO: cleanup task
                return NULL;
            }
            // Allocate physical page
            uint32_t phys = pmm_alloc_page();
            if (phys == 0) {
                printf("ELF: Failed to allocate page for segment %u\n", i);
                paging_switch_directory(old_dir);
                // TODO: cleanup task
                return NULL;
            }

            // Map at the requested virtual address
            if (!paging_map_page(vaddr, phys, segment.page_flags)) {
                printf("ELF: Failed to map page at %#x\n", vaddr);
                pmm_free_page(phys);
                paging_switch_directory(old_dir);
                // TODO: cleanup task
                return NULL;
            }

            // Now page is accessible at vaddr - copy data to it
            uint32_t page_offset = vaddr - segment.vaddr;
            if (page_offset < segment.filesz) {
                // This page contains file data
                uint32_t bytes_to_copy = PAGE_SIZE;
                if (page_offset + bytes_to_copy > segment.filesz) {
                    bytes_to_copy = segment.filesz - page_offset;
                }
                memcpy(
                    (void*)vaddr,
                    elf_data + segment.offset + page_offset,
                    bytes_to_copy
                );
                // Zero BSS portion if any
                if (bytes_to_copy < PAGE_SIZE) {
                    memset(
                        (void*)(vaddr + bytes_to_copy),
                        0,
                        PAGE_SIZE - bytes_to_copy
                    );
                }
            } else {
                // Pure BSS - zero entire page
                memset((void*)vaddr, 0, PAGE_SIZE);
            }
        }

        task_add_mem_region(task, vaddr_start, vaddr_end, segment.page_flags);
    }

    // Set up the heap after highest virt address, page aligned
    uint32_t heap_start = PAGE_ALIGN_UP(parsed->max_vaddr);
    uint32_t stack_bottom = task->user_stack_base;
    uint32_t heap_max = stack_bottom - MIN_HEAP_STACK_GAP;
    if (heap_start >= heap_max) {
        puts("ELF: no space for heap (segments too large");
        paging_switch_directory(old_dir);
        // TODO: cleanup
        return NULL;
    }
    task->heap_start = heap_start;
    task->heap_end = heap_start; // Empty initially
    task->heap_max = heap_max;
    printf(
        "Task [%u] heap: start=%#x end=%#x max=%#x (available: %u KiB)\n",
        task->pid,
        task->heap_start,
        task->heap_end,
        task->heap_max,
        (task->heap_max - task->heap_start) / 1024
    );

    // Switch back to kernel directory
    paging_switch_directory(old_dir);

    kfree(parsed);
    return task;
}
