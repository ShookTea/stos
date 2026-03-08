#include "gdt.h"
#include <stdint.h>

gdt_entry_t gdt_entries[5];
gdt_ptr_t gdt_ptr;

/**
 * Load GDT pointer address to the registry
 */
extern void gdt_flush(uint32_t);

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

void init_gdt(void)
{
    // Load limit and base to GDT pointer
    gdt_ptr.limit = (sizeof(gdt_entry_t) * 5) - 1;
    gdt_ptr.base = (uint32_t)&gdt_entries;

    // Populate GDT entries:
    gdt_set_entry(0, 0, 0, 0, 0); // null segment
    gdt_set_entry(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // kernel code segment
    gdt_set_entry(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // kernel data segment
    gdt_set_entry(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // user mode code segment
    gdt_set_entry(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // user mode data segment

    // Load GDT to registries
    gdt_flush((uint32_t)&gdt_ptr);
}
