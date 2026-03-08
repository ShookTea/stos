#include "idt.h"
#include <stdbool.h>
#include <kernel/tty.h>

// An array of IDT entries
__attribute__((aligned(0x10)))
static idt_entry_t idt[IDT_MAX_DESCRIPTORS];
static idt_ptr_t idt_ptr;
static bool vectors[IDT_MAX_DESCRIPTORS];

extern void* isr_stub_table[];

static void idt_set_descriptor(uint8_t vector, void* isr, uint8_t flags)
{
    idt[vector].offset_low = (uint32_t) isr & 0xFFFF;
    idt[vector].offset_high = (uint32_t) isr >> 16;
    idt[vector].segment_selector = 0x08; // kernel code selector
    idt[vector].reserved = 0;
    idt[vector].flags = flags;
}

void init_idt(void)
{
    idt_ptr.offset = (uint32_t)&idt;
    idt_ptr.size = (uint16_t)sizeof(idt_entry_t) * IDT_MAX_DESCRIPTORS - 1;

    // Load IDT vectors for basic 32 interrupts:
    for (uint8_t vector = 0; vector < 32; vector++) {
        idt_set_descriptor(vector, isr_stub_table[vector], 0x8E);
        vectors[vector] = true;
    }

    // Load the new IDT
    __asm__ volatile ("lidt %0" : : "m"(idt_ptr));
    // Enable interrupts
    __asm__ volatile ("sti");
}

/**
 * General exception handler that will completely hang the computer.
 * Temporarily add ignore for warning - compiler doesn't understand that this
 * ASM code will, in fact, not return.
 */
void exception_handler(void)
{
    tty_writestring("Halted"); // TODO: remove this line
    __asm__ volatile ("cli; hlt");
}
