#include "kernel/memory/kmalloc.h"
#include <kernel/vfs/vfs.h>
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

    puts("SHT table:");
    for (uint32_t shti = 0; shti < header->sht_num; shti++) {
        elf_section_header_32bit_t* sht = (elf_section_header_32bit_t*)(
            addr + header->sht_pointer + (header->sht_entry_size * shti)
        );
        if (sht->type == ELF_SECTION_TYPE_NULL) {
            continue;
        }
        char* type;
        char* flags = "          ";
        switch (sht->type) {
            case ELF_SECTION_TYPE_PROGBITS: type = "program data"; break;
            case ELF_SECTION_TYPE_SYMTAB: type = "symbol table"; break;
            case ELF_SECTION_TYPE_STRTAB: type = "string table"; break;
            case ELF_SECTION_TYPE_RELA: type = "relococation entries"; break;
            case ELF_SECTION_TYPE_HASH: type = "symbol hash table"; break;
            case ELF_SECTION_TYPE_DYNAMIC: type = "dyn. linkining info"; break;
            case ELF_SECTION_TYPE_NOTE: type = "notes"; break;
            case ELF_SECTION_TYPE_NOBITS: type = "bss"; break;
            case ELF_SECTION_TYPE_REL: type = "relocation entries"; break;
            case ELF_SECTION_TYPE_SHLIB: type = "shlib (reserved)"; break;
            case ELF_SECTION_TYPE_DYNSYM: type = "dyn. link. symbol tbl"; break;
            case ELF_SECTION_TYPE_INIT_ARRAY: type = "init array"; break;
            case ELF_SECTION_TYPE_FINI_ARRAY: type = "fini array"; break;
            case ELF_SECTION_TYPE_PREINIT_ARRAY: type = "preinit_array"; break;
            case ELF_SECTION_TYPE_GROUP: type = "group"; break;
            case ELF_SECTION_TYPE_SYMTAB_SHNDX: type = "ext. sect. indx"; break;
            case ELF_SECTION_TYPE_NUM: type = "number of defined types"; break;
            default: type = "unrecognized"; return false;
        }
        flags[0] = sht->flags & ELF_SECTION_FLAGS_WRITE ? 'W' : ' ';
        flags[1] = sht->flags & ELF_SECTION_FLAGS_ALLOC ? 'A' : ' ';
        flags[2] = sht->flags & ELF_SECTION_FLAGS_EXEC ? 'X' : ' ';
        flags[3] = sht->flags & ELF_SECTION_FLAGS_MERGE ? 'M' : ' ';
        flags[4] = sht->flags & ELF_SECTION_FLAGS_STRINGS ? 'S' : ' ';
        flags[5] = sht->flags & ELF_SECTION_FLAGS_INFO_LINK ? 'L' : ' ';
        flags[6] = sht->flags & ELF_SECTION_FLAGS_LINK_ORDR ? 'O' : ' ';
        flags[7] = sht->flags & ELF_SECTION_FLAGS_OS_NONCMF ? 'N' : ' ';
        flags[8] = sht->flags & ELF_SECTION_FLAGS_GROUP ? 'G' : ' ';
        flags[9] = sht->flags & ELF_SECTION_FLAGS_TLS ? 'T' : ' ';

        printf("[%02u] Type: [%s] %s\n", shti, flags, type);
        printf("    addr:      %#x\n", sht->addr);
        printf("    offset:    %#x\n", sht->offset);
        printf("    size:      %#x\n", sht->size);
        printf("    link:      %#x\n", sht->link);
        printf("    info:      %#x\n", sht->info);
        printf("    align:     %#x\n", sht->addr_align);
        printf("    ent. size: %#x\n", sht->ent_size);
    }

    return true;
}

void elf_dump(vfs_file_t* handle)
{
    void* file = kmalloc_flags(handle->node->length, KMALLOC_ZERO);
    vfs_read(handle, handle->node->length, file);
    vfs_close(handle);
    // For now the validation code prints all the debug info
    elf_validate(file);
    kfree(file);
}
