#include <kernel/elf.h>
#include <stdbool.h>
#include <stdio.h>


bool elf_validate(void* addr)
{
    elf_header_32bit_t* header = addr;
    if (header->magic != 0x7F454C46) {
        printf("Validation failed: wrong magic value (%#x)\n", header->magic);
        return false;
    }

    return true;
}
