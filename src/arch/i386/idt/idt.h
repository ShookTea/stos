#ifndef ARCH_I386_IDT_H
#define ARCH_I386_IDT_H

#include <stdint.h>

#define IDT_MAX_DESCRIPTORS 256

/*
 * A required shift for IRQ interrupts. For example, IRQ 1 (PS/2 keyboard)
 * is at (1 + 32) = int 33
 */
#define IDT_IRQ_INTERRUPT_SHIFT 32

/**
 * IDT entry structure, bit by bit:
 * - [00-15] offset_low (lower 16 bits of offset)
 * - [16-31] segment_selector
 * - [32-39] reserved byte
 * - [40-47] flags
 * - [48-63] offset_high (upper 16 bits of offset)
 *
 * *Offset* is a 32-bit value that represents the entry point of the ISR
 * (Interrupt Service Routine)
 *
 * *Segment selector* is the segment selector that has to point to a valid
 * memory segment (kernel code segment) in the GDT table.
 *
 * *flags* are 8 bits, from highest to lowest:
 * - [ 7 ] *P* - present bit. Must be set (1) for descriptor to be valid
 * - [6-5] *DPL* - a 2-bit value defining a CPU privilege level (ring) which is
 *         allowed to access this interrupt via INT instruction. Hardware
 *         interrupts ignore this mechanism.
 * - [ 4 ] reserved
 * - [3-0] gate type that defines a type of gate represented by this entry.
 *         five values are valid:
 *         - 0x5 - task gate
 *         - 0x6 - 16-bit interrupt gate
 *         - 0x7 - 16-bit trap gate
 *         - 0xE - 32-bit interrupt gate
 *         - 0xF - 32-bit trap gate
 */
typedef struct
{
    uint16_t offset_low;
    uint16_t segment_selector;
    uint8_t reserved;
    uint8_t flags;
    uint16_t offset_high;
} __attribute__((packed)) idt_entry_t;

/**
 * Pointer to the entire IDT table:
 * - 32-bit offset points to a linear address of the first entry of IDT - not the
 *   physical address, paging applies
 * - 16-bit size is the size (in bytes) of all IDT entries minus 1 byte.
 */
typedef struct
{
    uint16_t size;
    uint32_t offset;
} __attribute__((packed)) idt_ptr_t;

typedef struct
{
   uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha.
   uint32_t int_no; // Interrupt number
   uint32_t err_code; // Error code (if applicable)
   uint32_t eip, cs, eflags, useresp, ss; // Pushed by the processor automatically.
} registers_t;

// int_handler_t is a function, receiving registers_t as an argument, that can
// be used for registering interrupt handlers
typedef void (*int_handler_t)(registers_t*);

void init_idt(void);

/**
 * Register interrupt handler for a given IRQ number.
 */
void idt_register_irq_handler(uint8_t int_number, int_handler_t handler);

#endif
