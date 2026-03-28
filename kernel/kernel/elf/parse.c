#include "kernel/paging.h"
#include <kernel/elf.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void elf_parse(void* addr, elf_t* store)
{
    memset(store, 0, sizeof(elf_t));
    if (!elf_validate(addr)) {
        puts("Invalid ELF file.");
        store->success = false;
        return;
    }
    elf_header_32bit_t* header = addr;
    store->entry_point = header->entrypoint;
    uint32_t min_vaddr = UINT32_MAX;
    uint32_t max_vaddr = 0;

    size_t idx = 0;
    for (uint32_t phti = 0; phti < header->pht_num; phti++) {
        elf_program_header_32bit_t* pht = (elf_program_header_32bit_t*)(
            addr + header->pht_pointer + (header->pht_entry_size * phti)
        );
        if (pht->type != ELF_SEGMENT_TYPE_LOAD) {
            continue;
        }

        store->segments[idx].vaddr = pht->vaddr;
        store->segments[idx].memsz = pht->memsz;
        store->segments[idx].filesz = pht->filesz;
        store->segments[idx].offset = pht->offset;

        if (pht->vaddr < min_vaddr) {
            min_vaddr = pht->vaddr;
        }
        if ((pht->vaddr + pht->memsz) > max_vaddr) {
            max_vaddr = pht->vaddr + pht->memsz;
        }

        store->segments[idx].page_flags = PAGE_PRESENT | PAGE_USER;
        if (pht->flags & ELF_SEGMENT_FLAGS_WRITE) {
            store->segments[idx].page_flags |= PAGE_WRITE;
        }
        idx++;
    }

    printf("min=%#x, max=%#x\n", min_vaddr, max_vaddr);

    store->success = true;
    store->segment_count = idx;
    store->min_vaddr = min_vaddr;
    store->max_vaddr = max_vaddr;
}
