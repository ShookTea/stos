#include <kernel/elf.h>
#include <kernel/memory/pmm.h>
#include <kernel/paging.h>
#include <kernel/memory/vmm.h>
#include <stdio.h>
#include <string.h>

static inline bool in_userspace(uint32_t addr)
{
    return addr < VMM_USER_END;
}

bool elf_load_segments(void* elf_data, elf_t* parsed)
{
    if (!in_userspace(parsed->entry_point)) {
        printf(
            "ELF invalid: entrypoint outside of user space (%#x)\n",
            parsed->entry_point
        );
        return false;
    }

    for (size_t i = 0; i < parsed->segment_count; i++) {
        uint32_t start = parsed->segments[i].vaddr;
        uint32_t end = start + parsed->segments[i].memsz;
        if (!in_userspace(start) || !in_userspace(end)) {
            printf(
                "ELF invalid: segment %u outside of user space\n",
                (unsigned)i
            );
            return false;
        }
    }

    for (size_t i = 0; i < parsed->segment_count; i++) {
        elf_segment_t segment = parsed->segments[i];

        uint32_t vaddr_start = PAGE_ALIGN_DOWN(segment.vaddr);
        uint32_t vaddr_end = PAGE_ALIGN_UP(segment.vaddr + segment.memsz);
        uint32_t num_pages = (vaddr_end - vaddr_start) / PAGE_SIZE;

        for (uint32_t p = 0; p < num_pages; p++) {
            uint32_t vaddr = vaddr_start + (p * PAGE_SIZE);

            if (paging_is_mapped(vaddr)) {
                printf("ELF error: vaddr %#x is already mapped\n", vaddr);
                return false;
            }

            uint32_t phys = pmm_alloc_page();
            if (phys == 0) {
                printf(
                    "ELF: Failed to allocate page for segment %u\n",
                    (unsigned)i
                );
                return false;
            }

            uint32_t load_flags = segment.page_flags | PAGE_WRITE;
            if (!paging_map_page(vaddr, phys, load_flags)) {
                printf("ELF: Failed to map page at %#x\n", vaddr);
                pmm_free_page(phys);
                return false;
            }

            // page_offset is relative to segment.vaddr; for pages before the
            // segment start (alignment padding) this underflows to a large
            // value which correctly falls through to the BSS zero branch.
            uint32_t page_offset = vaddr - segment.vaddr;
            if (page_offset < segment.filesz) {
                uint32_t bytes_to_copy = PAGE_SIZE;
                if (page_offset + bytes_to_copy > segment.filesz) {
                    bytes_to_copy = segment.filesz - page_offset;
                }
                memcpy(
                    (void*)vaddr,
                    elf_data + segment.offset + page_offset,
                    bytes_to_copy
                );
                if (bytes_to_copy < PAGE_SIZE) {
                    memset(
                        (void*)(vaddr + bytes_to_copy),
                        0,
                        PAGE_SIZE - bytes_to_copy
                    );
                }
            } else {
                memset((void*)vaddr, 0, PAGE_SIZE);
            }

            if (!(segment.page_flags & PAGE_WRITE)) {
                paging_map_page(vaddr, phys, segment.page_flags);
            }
        }
    }

    return true;
}
