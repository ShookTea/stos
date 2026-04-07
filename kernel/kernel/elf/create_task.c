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

task_t* elf_create_task(const char* name, void* elf_data)
{
    elf_t* parsed = kmalloc(sizeof(elf_t));
    elf_parse(elf_data, parsed);
    if (!parsed->success) {
        kfree(parsed);
        return NULL;
    }

    printf("ELF: Entry point: %#x\n", parsed->entry_point);
    printf(
        "ELF: min_vaddr=%#x, max_vaddr=%#x\n",
        parsed->min_vaddr,
        parsed->max_vaddr
    );

    task_t* task = task_create(name, (void (*)())parsed->entry_point, false);
    void* old_dir = paging_get_current_directory();
    paging_switch_directory(task->page_dir_virt);

    if (!elf_load_segments(elf_data, parsed)) {
        paging_switch_directory(old_dir);
        kfree(parsed);
        // TODO: cleanup task
        return NULL;
    }

    // Add memory regions for each loaded segment
    for (size_t i = 0; i < parsed->segment_count; i++) {
        elf_segment_t* seg = &parsed->segments[i];
        task_add_mem_region(
            task,
            PAGE_ALIGN_DOWN(seg->vaddr),
            PAGE_ALIGN_UP(seg->vaddr + seg->memsz),
            seg->page_flags
        );
    }

    // Set up the heap after highest virt address, page aligned
    uint32_t heap_start = PAGE_ALIGN_UP(parsed->max_vaddr);
    uint32_t stack_bottom = task->user_stack_base;
    uint32_t heap_max = stack_bottom - MIN_HEAP_STACK_GAP;
    if (heap_start >= heap_max) {
        puts("ELF: no space for heap (segments too large");
        paging_switch_directory(old_dir);
        kfree(parsed);
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

    paging_switch_directory(old_dir);
    kfree(parsed);
    return task;
}
