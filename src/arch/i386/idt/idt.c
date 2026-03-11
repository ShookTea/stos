#include "idt.h"
#include <stdbool.h>
#include <stdio.h>
#include "pic.h"

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
    // Remap the IRQ table
    pic_remap(0x20, 0x28);

    idt_ptr.offset = (uint32_t)&idt;
    idt_ptr.size = (uint16_t)sizeof(idt_entry_t) * IDT_MAX_DESCRIPTORS - 1;

    // Load IDT vectors for:
    // - basic 32 interrupts
    // - 16 IRQ interrupts
    uint8_t total_vectors = 32 + 16;
    for (uint8_t vector = 0; vector < total_vectors; vector++) {
        idt_set_descriptor(vector, isr_stub_table[vector], 0x8E);
        vectors[vector] = true;
    }

    // Load the new IDT
    __asm__ volatile ("lidt %0" : : "m"(idt_ptr));
    // Enable interrupts
    __asm__ volatile ("sti");
}

/**
 * Handle page fault exceptions (interrupt 14) with detailed diagnostics
 */
static void handle_page_fault(registers_t* registers)
{
    // Read CR2 register to get the faulting address
    uint32_t faulting_address;
    __asm__ volatile("mov %%cr2, %0" : "=r"(faulting_address));

    printf("\n=== PAGE FAULT ===\n");
    printf("Page Fault at address: %#x\n", faulting_address);
    printf("Error code: %#x\n", registers->err_code);

    // Decode error code bits
    printf("  %s\n", (registers->err_code & 0x1) ?
           "Page protection violation" : "Page not present");
    printf("  %s operation\n", (registers->err_code & 0x2) ?
           "Write" : "Read");
    printf("  %s mode\n", (registers->err_code & 0x4) ?
           "User" : "Supervisor");

    if (registers->err_code & 0x8) {
        printf("  Reserved bit set in page table entry\n");
    }

    if (registers->err_code & 0x10) {
        printf("  Instruction fetch\n");
    }

    printf("\nRegisters at fault:\n");
    printf("  EIP: %#x  ESP: %#x  EBP: %#x\n",
           registers->eip, registers->esp, registers->ebp);
    printf("  EAX: %#x  EBX: %#x  ECX: %#x  EDX: %#x\n",
           registers->eax, registers->ebx, registers->ecx, registers->edx);
    printf("  ESI: %#x  EDI: %#x\n",
           registers->esi, registers->edi);
    printf("  CS: %#x  EFLAGS: %#x\n",
           registers->cs, registers->eflags);

    printf("\nSystem halted.\n");
    __asm__ volatile ("cli; hlt");
}

/**
 * General exception handler
 * Handles IRQs and CPU exceptions, with special handling for page faults
 */
void exception_handler(registers_t* registers)
{
    if (registers->int_no >= 0x20) {
        printf(
            "Received interrupt at IRQ %u\n",
            registers->int_no - 0x20
        );
        pic_send_eoi(registers->int_no - 0x20);
    }
    else if (registers->int_no == 14) {
        // Page fault exception - handle with detailed diagnostics
        handle_page_fault(registers);
    }
    else {
        printf(
            "Halted with exception int %#02X, err code %#X\n",
            registers->int_no,
            registers->err_code
        );
        __asm__ volatile ("cli; hlt");
    }
}
