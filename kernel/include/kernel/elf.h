#ifndef KERNEL_ELF_H
#define KERNEL_ELF_H

#include <kernel/vfs/vfs.h>
#include <kernel/task/task.h>
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

#define ELF_SECTION_TYPE_NULL           0x00 // Unused
#define ELF_SECTION_TYPE_PROGBITS       0x01 // Program data
#define ELF_SECTION_TYPE_SYMTAB         0x02 // Symbol table
#define ELF_SECTION_TYPE_STRTAB         0x03 // String table
#define ELF_SECTION_TYPE_RELA           0x04 // Relocation entries with addends
#define ELF_SECTION_TYPE_HASH           0x05 // Symbol hash table
#define ELF_SECTION_TYPE_DYNAMIC        0x06 // Dynamic linking info
#define ELF_SECTION_TYPE_NOTE           0x07 // Notes
#define ELF_SECTION_TYPE_NOBITS         0x08 // BSS (program space, no data)
#define ELF_SECTION_TYPE_REL            0x09 // Relocation entries, no addends
#define ELF_SECTION_TYPE_SHLIB          0x0A // Reserved
#define ELF_SECTION_TYPE_DYNSYM         0x0B // Dynamic linker symbol table
#define ELF_SECTION_TYPE_INIT_ARRAY     0x0E // Array of constructors
#define ELF_SECTION_TYPE_FINI_ARRAY     0x0F // Array of destructors
#define ELF_SECTION_TYPE_PREINIT_ARRAY  0x10 // Array of pre-constructors
#define ELF_SECTION_TYPE_GROUP          0x11 // Section group
#define ELF_SECTION_TYPE_SYMTAB_SHNDX   0x12 // Extended section indices
#define ELF_SECTION_TYPE_NUM            0x13 // Number of defined types

#define ELF_SECTION_FLAGS_WRITE     0x0001 // Writeable
#define ELF_SECTION_FLAGS_ALLOC     0x0002 // Occupies memory during execution
#define ELF_SECTION_FLAGS_EXEC      0x0004 // Executable
#define ELF_SECTION_FLAGS_MERGE     0x0010 // Might be merged
#define ELF_SECTION_FLAGS_STRINGS   0x0020 // Contains null-terminated strings
#define ELF_SECTION_FLAGS_INFO_LINK 0x0040 // sh_info contains SHT index
#define ELF_SECTION_FLAGS_LINK_ORDR 0x0080 // Preserve order after combining
#define ELF_SECTION_FLAGS_OS_NONCMF 0x0100 // non-standard specifict handling
#define ELF_SECTION_FLAGS_GROUP     0x0200 // Member of a group
#define ELF_SECTION_FLAGS_TLS       0x0400 // Thread-local data

/**
 * ELF file structure, for 32-bit systems
 */
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
    uint32_t flags;
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

typedef struct {
    /**
     * offset to string in SHT section name table that represents the name of
     * this section
     */
    uint32_t name;
    uint32_t type; // One of ELF_SECTION_TYPE_ values
    uint32_t flags; // Section attributes (ELF_SECTION_FLAGS_)
    uint32_t addr; // Virt. address of section, for those that are loaded
    uint32_t offset; // Offset of the section in the file image
    uint32_t size; // Size of bytes in the section (may be 0)
    uint32_t link; // Section index of an associated section (depends on type)
    uint32_t info; // Extra information (depends on type)
    uint32_t addr_align; // Required alignment of the section
    /** Size of each entry, for sections that contain fixed-size entries */
    uint32_t ent_size;
} elf_section_header_32bit_t;

/**
 * Structure representing a single loadable segment info after parsing.
 */
typedef struct {
    uint32_t vaddr; // virtual address to load at
    uint32_t memsz; // Size in memory
    uint32_t filesz; // Size in file (may be < memsz for BSS sections)
    uint32_t offset; // Offset in ELF file
    uint32_t page_flags; // PAGE_PRESENT | PAGE_WRITE | PAGE_USER etc
} elf_segment_t;

/**
 * Parsed structure of the ELF file
 */
typedef struct {
    bool success; // Whether the file was parsed successfully or not
    uint32_t entry_point;
    uint32_t min_vaddr; // Lowest virt. address used
    uint32_t max_vaddr; // Highest virt. address used
    size_t segment_count;
    elf_segment_t segments[32]; // Max. 32 loadable segments
} elf_t;

/**
 * Validates if data stored at [addr] is a valid ELF file that is understood
 * by the kernel.
 */
bool elf_validate(void* addr);

/**
 * Loads content of the file, treating it as an ELF file, and dumps information
 */
void elf_dump(vfs_file_t* file);

/**
 * Loads ELF from address and parses it for execution
 */
void elf_parse(void* addr, elf_t* store);

/**
 * Load all PT_LOAD segments from a parsed ELF into the currently active page
 * directory. Validates that all addresses are in user space. Does not modify
 * any task state (memory regions, heap, etc.) — the caller is responsible for
 * that. Returns true on success.
 */
bool elf_load_segments(void* elf_data, elf_t* parsed);

/**
 * Creates a new task from ELF binary data
 */
task_t* elf_create_task(
    const char* name,
    void* elf_data,
    vfs_node_t* root_dir,
    vfs_node_t* working_dir
);

#endif
