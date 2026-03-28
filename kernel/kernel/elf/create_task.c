#include <stdlib.h>
#include <kernel/memory/kmalloc.h>
#include <kernel/memory/vmm.h>
#include <kernel/task/task.h>
#include <kernel/elf.h>
#include <stdio.h>

static inline bool in_userspace(uint32_t addr)
{
    return addr < VMM_USER_END;
}

static inline bool memory_overlap(
    uint32_t start_1,
    uint32_t end_1,
    uint32_t start_2,
    uint32_t end_2
) {
    if (start_1 > end_1 || start_2 > end_2) {
        printf(
            "Invalid values: s1=%#x e1=%#x s2=%#x e2=%#x\n",
            start_1,
            end_1,
            start_2,
            end_2
        );
        abort();
    }
    return !(end_1 < start_2 || end_2 < start_1);
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
        if (memory_overlap(start, end, VMM_KERNEL_HEAP, VMM_USER_START)) {
            printf(
                "%s seg. %u (%#x-%#x) overlaps with kernel heap (%#x-%#x)\n",
                "ELF invalid:",
                i,
                start,
                end,
                VMM_KERNEL_HEAP,
                VMM_USER_START
            );
        }
    }

    return NULL;
}
