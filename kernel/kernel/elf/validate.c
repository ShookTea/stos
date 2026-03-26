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

    puts("ELF header validation completed successfully.");

    switch (header->obj_type) {
        case ELF_OBJ_TYPE_EXEC:
            puts("Type: executable file");
            break;
        case ELF_OBJ_TYPE_DYN:
            puts("Type: shared object");
            return false; // Not supported yet
        default:
            printf("Type: unknown (%#4x)\n", header->obj_type);
            return false;
    }

    puts("PHT table:");
    for (uint32_t phti = 0; phti < header->pht_num; phti++) {
        elf_program_header_32bit_t* pht = (elf_program_header_32bit_t*)(
            addr + header->pht_pointer + (header->pht_entry_size * phti)
        );
        if (pht->type == ELF_SEGMENT_TYPE_NULL) {
            continue;
        }
        char* type;
        char* flags = "   ";
        switch (pht->type) {
            case ELF_SEGMENT_TYPE_LOAD: type = "loadable"; break;
            case ELF_SEGMENT_TYPE_DYNAMIC: type = "dynamic linking info"; break;
            case ELF_SEGMENT_TYPE_INTERP: type = "interpreter info"; break;
            case ELF_SEGMENT_TYPE_NOTE: type = "aux. info"; break;
            default: type = "unrecognized"; return false;
        }
        flags[0] = pht->flags & ELF_SEGMENT_FLAGS_EXEC ? 'X' : ' ';
        flags[1] = pht->flags & ELF_SEGMENT_FLAGS_WRITE ? 'W' : ' ';
        flags[2] = pht->flags & ELF_SEGMENT_FLAGS_READ ? 'R' : ' ';
        printf("[%02u] Type: [%s] %s\n", phti, flags, type);
        printf("    offset: %#x\n", pht->offset);
        printf("    vaddr:  %#x\n", pht->vaddr);
        printf("    paddr:  %#x\n", pht->paddr);
        printf("    filesz: %#x\n", pht->filesz);
        printf("    memsz:  %#x\n", pht->memsz);
        printf("    align:  %#x\n", pht->align);
    }

    return true;
}
