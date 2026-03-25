#include "idt.h"
#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>
#include <kernel/paging.h>
#include <kernel/page_fault.h>
#include <kernel/task/syscall_handler.h>
#include "pic.h"

// An array of IDT entries
__attribute__((aligned(0x10)))
static idt_entry_t idt[IDT_MAX_DESCRIPTORS];
static idt_ptr_t idt_ptr;
static bool vectors[IDT_MAX_DESCRIPTORS];

extern void* isr_stub_table[];

static int_handler_t irq_int_handlers[16];
static syscall_int_handler_t syscall_int_handler;

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
    // Add syscall descriptor, placed at the end of isr_stub_table
    // TODO: switch flags from 0x8E to 0xEE for actual usermode support
    idt_set_descriptor(0x80, isr_stub_table[total_vectors], 0x8E);

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

    // Analyze the page fault
    page_fault_info_t pf_info;
    page_fault_analyze(
        faulting_address,
        registers->err_code,
        registers->eip,
        registers->esp,
        registers->ebp,
        &pf_info
    );

    // Print detailed information
    page_fault_print_info(&pf_info);

    // Print full register dump
    printf("\nComplete Register Dump:\n");
    printf("  EAX=%#010x  EBX=%#010x  ECX=%#010x  EDX=%#010x\n",
           registers->eax, registers->ebx, registers->ecx, registers->edx);
    printf("  ESI=%#010x  EDI=%#010x  EBP=%#010x  ESP=%#010x\n",
           registers->esi, registers->edi, registers->ebp, registers->esp);
    printf("  EIP=%#010x  EFLAGS=%#010x  CS=%#06x\n",
           registers->eip, registers->eflags, registers->cs);
    printf("  INT#=%#04x  ERR=%#010x\n",
           registers->int_no, registers->err_code);

    // Print stack trace
    page_fault_print_stack_trace(registers->ebp, 10);

    // Try to recover (currently always fails, but framework is in place)
    printf("\nAttempting recovery...\n");
    if (page_fault_try_recover(&pf_info)) {
        printf("Recovery successful! Continuing execution.\n");
        return;  // Return to faulting code - recovery succeeded
    }

    printf("\n========================================\n");
    printf("Cannot recover from page fault.\n");
    printf("System halted.\n");
    printf("========================================\n");

    // Print statistics before halting
    page_fault_print_stats();

    __asm__ volatile ("cli; hlt");
}

uint32_t syscall_handler_wrapper(
    uint32_t arg0,
    uint32_t arg1,
    uint32_t arg2,
    uint32_t arg3
) {
    if (syscall_int_handler != NULL) {
        return syscall_int_handler(
            arg0,
            arg1,
            arg2,
            arg3
        );
    } else {
        puts("Received unhandled syscall interrupt 0x80");
        return 0;
    }
}

/**
 * General exception handler
 * Handles IRQs and CPU exceptions, with special handling for page faults
 */
void exception_handler(registers_t* registers)
{
    if (registers->int_no >= IDT_IRQ_INTERRUPT_SHIFT) {
        uint8_t irq_number = registers->int_no - IDT_IRQ_INTERRUPT_SHIFT;
        if (irq_int_handlers[irq_number] != 0) {
            (irq_int_handlers[irq_number])(registers);
        } else {
            printf("Received unhandled interrupt at IRQ %u\n", irq_number);
        }
        pic_send_eoi(irq_number);
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

void idt_register_irq_handler(uint8_t int_number, int_handler_t handler)
{
    irq_int_handlers[int_number] = handler;
}

void syscall_handler_register(syscall_int_handler_t handler)
{
    syscall_int_handler = handler;
}
