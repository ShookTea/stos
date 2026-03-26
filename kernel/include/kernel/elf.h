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

#define ELF_SEGMENT_TYPE_NULL    0x00000000 // Unused
#define ELF_SEGMENT_TYPE_LOAD    0x00000001 // Loadable segment
#define ELF_SEGMENT_TYPE_DYNAMIC 0x00000002 // Dynamic linking info
#define ELF_SEGMENT_TYPE_INTERP  0x00000003 // Interpreter information
#define ELF_SEGMENT_TYPE_NOTE    0x00000004 // Auxiliary information

#define ELF_SEGMENT_FLAGS_EXEC  0x01 // Executable
#define ELF_SEGMENT_FLAGS_WRITE 0x02 // Writeable
#define ELF_SEGMENT_FLAGS_READ  0x04 // Readable

typedef struct {
    uint8_t magic[4]; // 0x7F followed by "ELF" (0x45 0x4C 0x46)
    uint8_t class; // set to 1 for 32-bit, 2 for 64-bit format
    uint8_t endianness; // 1 for little endian, 2 for big endian
    uint8_t version; // 1 = current version of ELF
    uint8_t os_abi; // target operating system ABI
    uint8_t os_abi_version;
    uint8_t pad[7]; // 7 reserved bytes, currently unused
    uint16_t obj_type; // One of ELF_OBJ_TYPE_ values
    uint16_t machine; // One of ELF_OBJ_MACHINE_ values
    uint32_t version2; // Set to 1 for original version
    uint32_t entrypoint;
    uint32_t pht_pointer; // Program Header Table pointer
    uint32_t sht_pointer; // Section Header Table pointer
    uint16_t flags;
    uint16_t header_size; // header size (typically 52 B for 32-bit)
    uint16_t pht_entry_size; // PHT entry size (typically 32 B for 32-bit)
    uint16_t pht_num; // Number of entries in PHT
    uint16_t sht_entry_size; // SHT entry size (typically 40 B for 32-bit)
    uint16_t sht_num; // Number of entries in SHT
    uint16_t sht_str_index; // Index of SHT that contains section names
} elf_header_32bit_t;

typedef struct {
    uint32_t type; // One of ELF_SEGMENT_TYPE_ values
    uint32_t offset; // Offset of the segment in the file image
    uint32_t vaddr; // Virtual address of the segment in memory
    uint32_t paddr; // Reserved for segment's physical address (if relevant)
    uint32_t filesz; // Size in bytes of the segment in the file img. (may be 0)
    uint32_t memsz; // Size in bytes of the segment in memory (may be 0)
    uint32_t flags; // Segment-dependent flags (ELF_SEGMENT_FLAGS_)
    /**
     * 0 and 1 specify no alignment; otherwise should be a positive integral
     * power of 2, with vaddr = offset modulus align
     */
    uint32_t align;
} elf_program_header_32bit_t;

/**
 * Validates if data stored at [addr] is a valid ELF file that is understood
 * by the kernel.
 */
bool elf_validate(void* addr);

#endif
