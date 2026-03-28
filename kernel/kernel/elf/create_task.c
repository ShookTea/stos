#include "kernel/memory/kmalloc.h"
#include "kernel/memory/vmm.h"
#include "kernel/task/task.h"
#include <kernel/elf.h>
#include <stdio.h>

inline bool in_userspace(uint32_t addr)
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
        if (!in_userspace(parsed->segments[i].vaddr)) {
            printf(
                "%s segment %u has addr outside of user space (%#x)\n",
                "ELF invalid:",
                i,
                parsed->segments[i].vaddr
            );
            return NULL;
        }
        if (!in_userspace(
            parsed->segments[i].vaddr + parsed->segments[i].memsz
        )) {
            printf(
                "%s segment %u has addr+size outside of user space (%#x)\n",
                "ELF invalid:",
                i,
                parsed->segments[i].vaddr + parsed->segments[i].memsz
            );
            return NULL;
        }
    }

    return NULL;
}
