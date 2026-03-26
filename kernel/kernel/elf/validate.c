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
            "Invalid ELF: wrong magic value (%#2x%2x%2x%2x)\n",
            header->magic[0],
            header->magic[1],
            header->magic[2],
            header->magic[3]
        );
        return false;
    }

    if (header->class != 1) {
        puts("Invalid ELF: only 32-bit apps are supported.");
        return false;
    }
    if (header->endianness != 1) {
        puts("Invalid ELF: only little endian apps are supported.");
        return false;
    }
    if (header->version != 1) {
        puts("Invalid ELF: only version 1 supported");
        return false;
    }
    if (header->machine != ELF_OBJ_MACHINE_X86) {
        puts("Invalid ELF: requires x86 architecture");
        return false;
    }

    return true;
}
