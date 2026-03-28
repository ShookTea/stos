#include <kernel/memory/pmm.h>
#include <kernel/paging.h>
#include <stdlib.h>
#include <kernel/memory/kmalloc.h>
#include <kernel/memory/vmm.h>
#include <kernel/task/task.h>
#include <kernel/elf.h>
#include <stdio.h>
#include <string.h>

static inline bool in_userspace(uint32_t addr)
{
    return addr < VMM_USER_END;
}

task_t* elf_create_task(const char* name, void* elf_data)
{
    elf_t* parsed = kmalloc(sizeof(elf_t));
    elf_parse(elf_data, parsed);

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
    paging_switch_directory(task->page_dir_phys);

    // Load all segments to the memory
    for (size_t i = 0; i < parsed->segment_count; i++) {
        elf_segment_t segment = parsed->segments[i];
        uint32_t size = segment.memsz;
        // Adding PMM_PAGE_SIZE-1 will guarantee that we will allocate required
        // number of pages. Without it, dividing i.e. 5000 / 4096 will give us
        // 1 page count instead of 2.
        uint32_t page_count = (size + PMM_PAGE_SIZE - 1) / PMM_PAGE_SIZE;
        void* allocation_addr = vmm_alloc(page_count, segment.page_flags);
        // Copy [filesz] bytes from ELF file to allocation address
        memcpy(
            allocation_addr,
            elf_data + segment.offset,
            segment.filesz
        );
        // Zero out BSS region (remaining memsz-filesz bytes)
        memset(
            allocation_addr + segment.filesz,
            0,
            segment.memsz - segment.filesz
        );
    }

    return task;
}
