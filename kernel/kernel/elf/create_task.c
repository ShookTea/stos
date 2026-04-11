#include <kernel/memory/pmm.h>
#include <kernel/paging.h>
#include <stdlib.h>
#include <kernel/memory/kmalloc.h>
#include <kernel/memory/vmm.h>
#include <kernel/task/task.h>
#include <kernel/elf.h>
#include "kernel/debug.h"
#include <string.h>

#define _debug_puts(...) debug_puts_c("ELF", __VA_ARGS__)
#define _debug_printf(...) debug_printf_c("ELF", __VA_ARGS__)

// Space between heap and stack (e.g. 1 MB)
#define MIN_HEAP_STACK_GAP (1 * 1024 * 1024) // 1 MB

task_t* elf_create_task(
    const char* name,
    void* elf_data,
    dentry_t* root_dir,
    dentry_t* working_dir
) {
    elf_t* parsed = kmalloc(sizeof(elf_t));
    elf_parse(elf_data, parsed);
    if (!parsed->success) {
        kfree(parsed);
        return NULL;
    }

    _debug_printf("Entry point: %#x\n", parsed->entry_point);
    _debug_printf(
        "min_vaddr=%#x, max_vaddr=%#x\n",
        parsed->min_vaddr,
        parsed->max_vaddr
    );

    task_t* task = task_create(
        name,
        (void (*)())parsed->entry_point,
        false,
        root_dir,
        working_dir
    );
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
        _debug_puts("no space for heap (segments too large");
        paging_switch_directory(old_dir);
        kfree(parsed);
        // TODO: cleanup
        return NULL;
    }
    task->heap_start = heap_start;
    task->heap_end = heap_start; // Empty initially
    task->heap_max = heap_max;
    _debug_printf(
        "Task [%u] heap: start=%#x end=%#x max=%#x (available: %u KiB)\n",
        task->pid,
        task->heap_start,
        task->heap_end,
        task->heap_max,
        (task->heap_max - task->heap_start) / 1024
    );

    /* Push argv/envp onto the new user stack while the task's page directory
     * is still active.  We pass the program name as argv[0] by default.
     *
     * The initial user ESP lives at a fixed offset inside the fake kernel-
     * stack interrupt frame built by task_setup_initial_stack().  Counting
     * upward from task->context.esp (which points at EDI callee-saved):
     *   [0..3]  callee-saved regs (EDI,ESI,EBX,EBP) for switch_to_stack
     *   [4]     task_switch_return_point
     *   [5..12] PUSHA frame (EDI,ESI,EBP,ESP,EBX,EDX,ECX,EAX)
     *   [13..16] segment regs (DS,ES,FS,GS)
     *   [17]    int_no
     *   [18]    err_code
     *   [19]    EIP
     *   [20]    CS
     *   [21]    EFLAGS
     *   [22]    user ESP   ← patch target
     *   [23]    SS
     *   [24]    task_entry_trampoline
     */
#define TASK_INITIAL_USER_ESP_OFFSET 22

    uint32_t user_esp = task->user_stack_base + task->user_stack_size;
    const char* default_argv[] = { name };
    const char* default_envp[] = { "foo=bar", NULL };
    task_push_args(&user_esp, 1, default_argv, 1, default_envp);
    ((uint32_t*)task->context.esp)[TASK_INITIAL_USER_ESP_OFFSET] = user_esp;

    paging_switch_directory(old_dir);
    kfree(parsed);
    return task;
}
