#include <kernel/elf.h>
#include <stdbool.h>
#include <stdio.h>


bool elf_validate(void* addr)
{
    elf_header_32bit_t* header = addr;
    // 0x7F454C46
    if (header->magic[0] != 0x7F
        || header->magic[1] != 'E'
        || header->magic[2] != 'L'
        || header->magic[3] != 'F'
    ) {
        printf(
            "Validation failed: wrong magic value (%#2x%2x%2x%2x)\n",
            header->magic[0],
            header->magic[1],
            header->magic[2],
            header->magic[3]
        );
        return false;
    }

    return true;
}
