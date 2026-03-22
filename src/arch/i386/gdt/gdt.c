#include "gdt.h"
#include <stdint.h>
#include <string.h>

static gdt_entry_t gdt_entries[6];
static gdt_ptr_t gdt_ptr;

// TSS structure
static tss_t tss;

/**
 * Load GDT pointer address to the registry
 */
extern void gdt_flush(uint32_t);

/**
 * Load TSS into task register - this tells the CPU where to find the TSS
 */
extern void tss_flush(void);

/**
 * Populate a selected GDT entry with given values
 */
static void gdt_set_entry(
    uint32_t index,
    uint32_t base,
    uint32_t limit,
    uint8_t access_flag,
    uint8_t granularity
) {
    // Load base (split between three parts)
    gdt_entries[index].base_low = (base & 0xFFFF);
    gdt_entries[index].base_middle = (base >> 16) & 0xFF;
    gdt_entries[index].base_high = (base >> 24) & 0xFF;

    // Load limit (split between limit_low and part of granularity)
    gdt_entries[index].limit_low = (limit & 0xFFFF);
    gdt_entries[index].granularity = (limit >> 16) & 0x0F;

    // Load granularity to the rest of the entry
    gdt_entries[index].granularity |= granularity & 0xF0;

    // Load access flags
    gdt_entries[index].access = access_flag;
}

static void init_tss()
{
    // Zero out the entire TSS
    memset(&tss, 0, sizeof(tss_t));
    // Set kernel stack segment
    tss.ss0 = 0x10; // Kernel data
    // ESP0 will be updated by tss_set_kernel_stack() during task switch
    tss.esp0 = 0;
    // Set up I/O map base (no I/O bitmap)
    tss.iomap_base = sizeof(tss_t);

    // Other fields remain zero - unused in software task switching
}

void init_gdt(void)
{
    // Load size and offset to GDT pointer
    gdt_ptr.size = (sizeof(gdt_entry_t) * 6) - 1;
    gdt_ptr.offset = (uint32_t)&gdt_entries;

    // Populate GDT entries:
    gdt_set_entry(0, 0, 0, 0, 0); // null segment
    gdt_set_entry(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // kernel code segment
    gdt_set_entry(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // kernel data segment
    gdt_set_entry(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // user mode code segment
    gdt_set_entry(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // user mode data segment
    gdt_set_entry(5, (uint32_t)&tss, sizeof(tss_t) - 1, 0x89, 0x00); // TSS

    init_tss();

    // Load GDT to registries
    gdt_flush((uint32_t)&gdt_ptr);

    // Load TSS (segment selector 0x28, with RPL=0)
    tss_flush();
}

void tss_set_kernel_stack(
    uint32_t kernel_stack_base,
    uint32_t kernel_stack_size
) {
    tss.esp0 = kernel_stack_base + kernel_stack_size;
}
