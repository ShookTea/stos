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

    return NULL;
}
