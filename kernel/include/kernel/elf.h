#ifndef KERNEL_ELF_H
#define KERNEL_ELF_H

#include <stdint.h>
#include <stdbool.h>

#define ELF_OBJ_TYPE_NONE 0x0000 // Unknown
#define ELF_OBJ_TYPE_REL  0x0001 // Relocatable file
#define ELF_OBJ_TYPE_EXEC 0x0002 // Executable file
#define ELF_OBJ_TYPE_DYN  0x0003 // Shared object

#define ELF_OBJ_MACHINE_UNKNOWN 0x0000 // No specific instruction set
#define ELF_OBJ_MACHINE_X86     0x0003 // x86

typedef struct {
    uint32_t magic; // 0x7F followed by "ELF" (0x45 0x4C 0x46)
    uint8_t class; // set to 1 for 32-bit, 2 for 64-bit format
    uint8_t endianness; // 1 for little endian, 2 for big endian
    uint8_t version; // 1 = current version of ELF
    uint8_t os_abi; // target operating system ABI
    uint8_t os_abi_version;
    uint8_t pad[7]; // 7 reserved bytes, currently unused
    uint16_t obj_type; // One of ELF_OBJ_TYPE_ values
    uint16_t machine; // One of ELF_OBJ_MACHINE_ values
    uint32_t version2; // Set to 1 for original version
} elf_header_32bit_t;

/**
 * Validates if data stored at [addr] is a valid ELF file that is understood
 * by the kernel.
 */
bool elf_validate(void* addr);

#endif
