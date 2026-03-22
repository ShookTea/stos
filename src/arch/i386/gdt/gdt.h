#ifndef ARCH_I386_GDT_H
#define ARCH_I386_GDT_H

#include <stdint.h>

/**
 * GDT entry structure, bit by bit:
 * - [00 - 15] limit_low (lower 16 bits of limit)
 * - [16 - 31] base_low (lowest 16 bits of base)
 * - [32 - 39] base_middle (middle 8 bits of base)
 * - [40 - 47] access byte flags
 * - [48 - 55] "granularity" - two values joined:
 *    - [48 - 51] limit_high (upper 4 bits of limit)
 *    - [52 - 55] granularity flags
 * - [56 - 63] base_high (upper 8 bits of base)
 *
 * *base* is a 32-bit value, containing the linear address where the segment
 * begins.
 *
 * *limit* is a 20-bit value that tells the maximum addressable unit, either
 * in 1 byte units or in 4 KiB pages, depending on the granularity flags. In
 * case of page granularity and limit set to 0xFFFFF the segment will span the
 * full 4 GiB address space (in 32-bit mode).
 *
 * *granularity flags* are 4 bits, from highest to lowest:
 * - [3] *G* - granularity. If clear, the limit is in 1-byte blocks, otherwise
 *       the limit is in 4 KiB blocks (page granularity)
 * - [2] *DB* - size flag. If clear, the description defines a 16-bit protected
 *       mode segment, otherwise it defines a 32-bit segment.
 * - [1] *L* - long-mode code flag: if set to 1, the descripor defines a 64-bit
 *       code segment. When set, DB should always be clear.
 * - [0] reserved
 *
 * *access byte flags* are 8 bits, from highest to lowest:
 * - [ 7 ] *P* - present bit. Must be set (1) for any valid segment.
 * - [6-5] *DPL* - descriptor CPU privilege level (ring). 0 = kernel, 3 = user
 *         applications.
 * - [ 4 ] *S* - type bit. If clear, it defines a system segment, otherwise it
 *         describes a code/data segment.
 * - [ 3 ] *E* - executable bit. If clear, defines a data segment, otherwise it
 *         defines a code segment.
 * - [ 2 ] *DC* - direction/conforming bit.
 *         - for data selectors: direction - if clear, the segment grows up,
 *           otherwise it grows down (so offset has to be greater than limit)
 *         - for code selectors: conforming - if clear, the code can only be
 *           executed from ring defined in DPL; otherwise it can be run in the
 *           same ring or one with lower privilage
 * - [ 1 ] *RW* - readable/writeable bit.
 *         - for data segment: writeable bit - if clear, write is not allowed,
 *           otherwise it's allowed. Reading is always allowed.
 *         - for code segment: readable bit - if clear, read access for this
 *           segment is forbidden, otherwise it's allowed. Writing is never
 *           allowed.
 * - [ 0 ] *A* - accessed bit. The CPU will set it when the segment is accessed,
 *         unlessed its set to 1 in advance. In case the GDT descriptor is
 *         stored in read-only page, it will cause a page fault, so it's better
 *         to set it to 1 in advance.
 */
typedef struct
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed)) gdt_entry_t;

/**
 * Pointer to the entire GDT table:
 * - 32-bit offset points to a linear address of the first entry of GDT - not the
 *   physical address, paging applies
 * - 16-bit size is the size (in bytes) of all GDT entries minus 1 byte.
 */
typedef struct
{
    uint16_t size;
    uint32_t offset;
} __attribute__((packed)) gdt_ptr_t;

/**
 * Task State Segment (TSS) for i386
 *
 * TSS is used by the CPU to find the kernel stack when switching from usermode
 * to kernel mode during interrupts, exceptions, or system calls.
 *
 * TODO: For now we're only supporting software task switching and keep a single
 * TSS (and update ESP0 when switching tasks), but x86 supports hardware task
 * switching, which uses different TSS for each task.
 */
typedef struct {
    /** Link to previous TSS, for hardware switching (not used yet) */
    uint32_t prev_tss;

    // Stack pointer and segment for ring 0
    /** Stack pointer for ring 0 */
    uint32_t esp0;
    /** Kernel data segment */
    uint32_t ss0;

    // Stack pointers and segments for ring 1&2 (not used by us)
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;

    /** Page directory register (CR3) */
    uint32_t cr3;

    // Registers for hardware task switching (not used yet)
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;

    // Segment registers
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;

    /** LDT segment selector (unused) */
    uint32_t ldt;
    /** Debug trap flag */
    uint16_t trap;
    /** I/O map base address */
    uint16_t iomap_base;
} __attribute__((packed)) tss_t;

/**
 * Initialize and load GDT. It should create 6 entries:
 * 1. The required null segment
 * 2. Kernel code segment
 * 3. Kernel data segment
 * 4. User mode code segment
 * 5. User mode data segment
 * 6. TSS segment
 * This is required for x86 architecture to work, but we're going to use
 * paging later anyway, so we should just make the null segment empty and
 * the rest of segments should fill the entire 4 GiB of RAM.
 */
void init_gdt(void);

#endif
