#include "stdlib.h"
#include <kernel/acpi.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "io.h"

// Global ACPI state
static acpi_fadt_t* g_fadt = NULL;
static bool acpi_enabled = false;
static bool acpi_available = false;
static uint8_t s5_slp_typa = 0;
static uint8_t s5_slp_typb = 0;
static bool s5_found = false;

// Calculate checksum for ACPI table
static bool validate_checksum(void* table, uint32_t length)
{
    uint8_t sum = 0;
    uint8_t* bytes = (uint8_t*)table;
    for (uint32_t i = 0; i < length; i++) {
        sum += bytes[i];
    }
    return sum == 0;
}

// Parse DSDT to find _S5 package for shutdown
static bool parse_s5_from_dsdt(void)
{
    if (!g_fadt || !g_fadt->dsdt) {
        puts("ACPI: No DSDT pointer in FADT");
        return false;
    }

    acpi_sdt_header_t* dsdt = (acpi_sdt_header_t*)g_fadt->dsdt;
    
    // Verify DSDT signature
    if (strncmp(dsdt->signature, "DSDT", 4) != 0) {
        puts("ACPI: Invalid DSDT signature");
        return false;
    }

    // Validate DSDT checksum
    if (!validate_checksum(dsdt, dsdt->length)) {
        puts("ACPI: DSDT checksum validation failed");
        return false;
    }

    printf("ACPI: Searching for _S5 in DSDT (length=%u)\n", dsdt->length);

    // Search for "_S5_" in DSDT
    uint8_t* start = (uint8_t*)dsdt + sizeof(acpi_sdt_header_t);
    uint8_t* end = (uint8_t*)dsdt + dsdt->length;
    
    for (uint8_t* ptr = start; ptr < end - 4; ptr++) {
        if (ptr[0] == '_' && ptr[1] == 'S' && ptr[2] == '5' && ptr[3] == '_') {
            printf("ACPI: Found _S5 at offset %u\n", (uint32_t)(ptr - (uint8_t*)dsdt));
            
            // This is a simplified AML parser - it may not work on all systems
            // A proper implementation would parse the full AML bytecode
            // For production, consider using uACPI library
            
            uint8_t* package_ptr = ptr + 4;
            
            // Skip to find package op (0x12)
            int search_limit = 16; // Don't search too far
            while (package_ptr < end && search_limit-- > 0) {
                if (*package_ptr == 0x12) { // Package op
                    break;
                }
                package_ptr++;
            }
            
            if (package_ptr >= end || *package_ptr != 0x12) {
                puts("ACPI: Could not find package op after _S5");
                continue;
            }
            
            // Parse package: [0x12] [PkgLength] [NumElements] [Elements...]
            package_ptr++; // Skip 0x12
            
            // PkgLength encoding can be 1-4 bytes
            uint8_t pkg_length_byte = *package_ptr++;
            if (pkg_length_byte & 0xC0) {
                // Multi-byte length, skip additional bytes
                int bytes_to_skip = (pkg_length_byte >> 6);
                package_ptr += bytes_to_skip;
            }
            
            uint8_t num_elements = *package_ptr++;
            
            if (num_elements < 2) {
                puts("ACPI: _S5 package has too few elements");
                continue;
            }
            
            // Extract SLP_TYPa and SLP_TYPb
            // Elements are typically ByteConst (0x0A) followed by value
            // or simple integers
            
            // First element (SLP_TYPa)
            if (*package_ptr == 0x0A) { // ByteConst
                package_ptr++;
                s5_slp_typa = *package_ptr++;
            } else if (*package_ptr <= 0x01) { // Zero or One
                s5_slp_typa = *package_ptr++;
            } else {
                s5_slp_typa = *package_ptr++;
            }
            
            // Second element (SLP_TYPb)
            if (*package_ptr == 0x0A) { // ByteConst
                package_ptr++;
                s5_slp_typb = *package_ptr++;
            } else if (*package_ptr <= 0x01) { // Zero or One
                s5_slp_typb = *package_ptr++;
            } else {
                s5_slp_typb = *package_ptr++;
            }
            
            // Mask to 3 bits (valid range 0-7)
            s5_slp_typa &= 0x07;
            s5_slp_typb &= 0x07;
            
            printf("ACPI: _S5 found - SLP_TYPa=%u, SLP_TYPb=%u\n", 
                   s5_slp_typa, s5_slp_typb);
            s5_found = true;
            return true;
        }
    }
    
    puts("ACPI: _S5 not found in DSDT");
    return false;
}

// Enable ACPI mode if not already enabled
static bool acpi_enable(void)
{
    if (acpi_enabled) {
        return true;
    }
    
    if (!g_fadt) {
        return false;
    }
    
    // If SMI_CMD is zero, ACPI is already enabled (or not supported)
    if (g_fadt->smi_command_port == 0) {
        printf("ACPI: SMI_CMD is 0, assuming ACPI already enabled\n");
        acpi_enabled = true;
        return true;
    }
    
    // Check if already in ACPI mode by checking SCI_EN bit
    if (inw(g_fadt->pm1a_control_block) & 0x0001) {
        printf("ACPI: SCI_EN already set\n");
        acpi_enabled = true;
        return true;
    }
    
    printf("ACPI: Enabling ACPI mode (SMI_CMD=%x, ACPI_ENABLE=%x)\n",
           g_fadt->smi_command_port, g_fadt->acpi_enable);
    
    // Write ACPI_ENABLE to SMI_CMD port
    outb(g_fadt->smi_command_port, g_fadt->acpi_enable);
    
    // Wait for SCI_EN bit to be set (with timeout)
    int timeout = 300; // 300 iterations
    while (timeout-- > 0) {
        if (inw(g_fadt->pm1a_control_block) & 0x0001) {
            printf("ACPI: ACPI mode enabled\n");
            acpi_enabled = true;
            return true;
        }
        
        // Small delay - in a real OS you'd use a timer
        for (volatile int i = 0; i < 10000; i++);
    }
    
    puts("ACPI: Failed to enable ACPI mode (timeout)");
    return false;
}

// Load and validate FADT
static void load_fadt(acpi_fadt_t* fadt)
{
    // Validate checksum
    if (!validate_checksum(fadt, fadt->header.length)) {
        puts("ACPI: Invalid FADT checksum");
        return;
    }
    
    printf("ACPI: FADT loaded (revision=%u, length=%u)\n", 
           fadt->header.revision, fadt->header.length);
    
    // Store FADT pointer
    g_fadt = fadt;
    acpi_available = true;
    
    // Print important FADT fields for debugging
    printf("ACPI: DSDT at 0x%x\n", fadt->dsdt);
    printf("ACPI: PM1a_CNT_BLK at 0x%x\n", fadt->pm1a_control_block);
    printf("ACPI: PM1b_CNT_BLK at 0x%x\n", fadt->pm1b_control_block);
    printf("ACPI: SMI_CMD at 0x%x\n", fadt->smi_command_port);
    printf("ACPI: ACPI_ENABLE=0x%x, ACPI_DISABLE=0x%x\n", 
           fadt->acpi_enable, fadt->acpi_disable);
    printf("ACPI: Flags=0x%x\n", fadt->flags);
    
    // Check reset register support
    if (fadt->flags & (1 << 10)) {
        printf("ACPI: Reset register supported (space=%u, addr=0x%llx, val=0x%x)\n",
               fadt->reset_reg.address_space,
               fadt->reset_reg.address,
               fadt->reset_value);
    }
}

static void load_rsdt_pointer(uint32_t pointer)
{
    acpi_sdt_header_t* header = (acpi_sdt_header_t*)pointer;
    if (strncmp(header->signature, "RSDT", 4) != 0) {
        puts("ACPI: Invalid RSDT signature");
        return;
    }
    
    // Validate RSDT checksum
    if (!validate_checksum(header, header->length)) {
        puts("ACPI: Invalid RSDT checksum");
        return;
    }
    
    printf("ACPI: RSDT valid (length=%u)\n", header->length);
    
    acpi_rsdt_t* rsdt = (acpi_rsdt_t*)header;
    uint32_t entries = (rsdt->header.length - sizeof(acpi_sdt_header_t)) / 4;
    
    printf("ACPI: RSDT contains %u entries\n", entries);
    
    for (uint32_t i = 0; i < entries; i++) {
        uint32_t sdt_pointer = rsdt->pointers_to_ther_sdt[i];
        acpi_sdt_header_t* sdt_header = (acpi_sdt_header_t*)sdt_pointer;
        
        printf("ACPI: Found table '%.4s' at 0x%x\n", sdt_header->signature, sdt_pointer);
        
        if (strncmp(sdt_header->signature, "FACP", 4) == 0) {
            load_fadt((acpi_fadt_t*)sdt_header);
        }
    }
}

void acpi_init_old(rsdp_old_t rsdp)
{
    // Validating RSDP signature
    if (memcmp(rsdp.signature, "RSD PTR ", 8) != 0)
    {
        puts("ACPI: Invalid RSDPv1 signature");
        return;
    }

    // Validating RSDP checksum - adding all unsigned bytes should return 0
    uint8_t checksum = rsdp.checksum + rsdp.revision;
    for (int i = 0; i < 8; i++) {
        checksum += rsdp.signature[i];
        if (i < 6) {
            checksum += rsdp.oemid[i];
        }
    }
    checksum += (uint8_t)(rsdp.rsdt_address & 0xFF);
    checksum += (uint8_t)((rsdp.rsdt_address >> 8) & 0xFF);
    checksum += (uint8_t)((rsdp.rsdt_address >> 16) & 0xFF);
    checksum += (uint8_t)((rsdp.rsdt_address >> 24) & 0xFF);

    if (checksum != 0) {
        puts("ACPI: Invalid RSDPv1 checksum");
        return;
    }
    
    printf("ACPI: RSDPv1 valid (RSDT at 0x%x)\n", rsdp.rsdt_address);
    load_rsdt_pointer(rsdp.rsdt_address);
}

void acpi_init_new(rsdp_new_t rsdp)
{
    // Validating RSDP signature
    if (memcmp(rsdp.signature, "RSD PTR ", 8) != 0)
    {
        puts("ACPI: Invalid RSDPv2 signature");
        return;
    }

    // Validating RSDP checksum - adding all unsigned bytes should return 0
    uint8_t checksum = rsdp.checksum + rsdp.revision;
    for (int i = 0; i < 8; i++) {
        checksum += rsdp.signature[i];
        if (i < 6) {
            checksum += rsdp.oemid[i];
        }
    }
    checksum += (uint8_t)(rsdp.__deprecated & 0xFF);
    checksum += (uint8_t)((rsdp.__deprecated >> 8) & 0xFF);
    checksum += (uint8_t)((rsdp.__deprecated >> 16) & 0xFF);
    checksum += (uint8_t)((rsdp.__deprecated >> 24) & 0xFF);

    if (checksum != 0) {
        puts("ACPI: Invalid RSDPv2 checksum");
        return;
    }

    // Validating extended checksum
    checksum += rsdp.extended_checksum;
    checksum += rsdp.reserved[0];
    checksum += rsdp.reserved[1];
    checksum += rsdp.reserved[2];

    checksum += (uint8_t)(rsdp.length & 0xFF);
    checksum += (uint8_t)((rsdp.length >> 8) & 0xFF);
    checksum += (uint8_t)((rsdp.length >> 16) & 0xFF);
    checksum += (uint8_t)((rsdp.length >> 24) & 0xFF);

    checksum += (uint8_t)(rsdp.rsdt_address & 0xFF);
    checksum += (uint8_t)((rsdp.rsdt_address >> 8) & 0xFF);
    checksum += (uint8_t)((rsdp.rsdt_address >> 16) & 0xFF);
    checksum += (uint8_t)((rsdp.rsdt_address >> 24) & 0xFF);
    checksum += (uint8_t)((rsdp.rsdt_address >> 32) & 0xFF);
    checksum += (uint8_t)((rsdp.rsdt_address >> 40) & 0xFF);
    checksum += (uint8_t)((rsdp.rsdt_address >> 48) & 0xFF);
    checksum += (uint8_t)((rsdp.rsdt_address >> 56) & 0xFF);

    if (checksum != 0) {
        puts("ACPI: Invalid RSDPv2 extended checksum");
        return;
    }

    puts("ACPI: RSDPv2 valid, but 64-bit mode not implemented yet.");
    puts("ACPI: Using deprecated 32-bit RSDT address for now");
    
    if (rsdp.__deprecated != 0) {
        load_rsdt_pointer(rsdp.__deprecated);
    }
}

// Keyboard controller reboot (fallback method)
static void keyboard_controller_reboot(void)
{
    puts("ACPI: Attempting keyboard controller reboot");
    
    // Disable interrupts
    asm volatile("cli");
    
    // Wait for keyboard controller to be ready
    uint8_t temp;
    int timeout = 1000;
    do {
        temp = inb(0x64);
        if (temp & 0x01) {
            inb(0x60); // Flush output buffer
        }
        if (--timeout == 0) {
            break;
        }
    } while (temp & 0x02);
    
    // Send reset command to keyboard controller
    outb(0x64, 0xFE);
    
    // Wait a bit
    for (volatile int i = 0; i < 1000000; i++);
    
    puts("ACPI: Keyboard controller reboot failed");
}

// Triple fault reboot (last resort)
static void triple_fault_reboot(void)
{
    puts("ACPI: Attempting triple fault");
    
    // Load invalid IDT and cause interrupt
    struct {
        uint16_t limit;
        uint32_t base;
    } __attribute__((packed)) invalid_idt = {0, 0};
    
    asm volatile("lidt %0" : : "m"(invalid_idt));
    asm volatile("int $0x00");
    
    // Should never reach here
    puts("ACPI: Triple fault failed");
}

// Public API: Reboot the system
void acpi_reboot(void)
{
    puts("ACPI: Attempting system reboot");
    
    // Method 1: ACPI Reset Register (ACPI 2.0+)
    if (g_fadt && (g_fadt->flags & (1 << 10))) {
        puts("ACPI: Using ACPI reset register");
        
        switch (g_fadt->reset_reg.address_space) {
            case 0: // System Memory
                printf("ACPI: Writing 0x%x to memory address 0x%llx\n",
                       g_fadt->reset_value, g_fadt->reset_reg.address);
                *((volatile uint8_t*)(uint32_t)g_fadt->reset_reg.address) = g_fadt->reset_value;
                break;
                
            case 1: // System I/O
                printf("ACPI: Writing 0x%x to I/O port 0x%llx\n",
                       g_fadt->reset_value, g_fadt->reset_reg.address);
                outb((uint16_t)g_fadt->reset_reg.address, g_fadt->reset_value);
                break;
                
            case 2: // PCI Configuration Space
                puts("ACPI: PCI reset not implemented");
                break;
                
            default:
                printf("ACPI: Unknown reset address space: %u\n", 
                       g_fadt->reset_reg.address_space);
                break;
        }
        
        // Wait for reset to take effect
        puts("ACPI: Waiting for reset...");
        for (volatile int i = 0; i < 10000000; i++);
        
        puts("ACPI: Reset register method failed");
    }
    
    // Method 2: Keyboard Controller Reset
    keyboard_controller_reboot();
    
    // Wait a bit
    for (volatile int i = 0; i < 1000000; i++);
    
    // Method 3: Triple Fault
    triple_fault_reboot();
    
    // If we get here, everything failed - just halt
    puts("ACPI: All reboot methods failed, halting");
    for(;;) {
        asm volatile("hlt");
    }
}

// Public API: Shutdown the system
void acpi_shutdown(void)
{
    puts("ACPI: Attempting system shutdown");
    
    if (!g_fadt) {
        puts("ACPI: FADT not available, cannot shutdown");
        puts("ACPI: System halted (power off manually)");
        asm volatile("cli");
        for(;;) {
            asm volatile("hlt");
        }
    }
    
    // Enable ACPI mode if not already enabled
    if (!acpi_enable()) {
        puts("ACPI: Failed to enable ACPI mode");
        puts("ACPI: System halted (power off manually)");
        asm volatile("cli");
        for(;;) {
            asm volatile("hlt");
        }
    }
    
    // Find _S5 sleep state if we haven't already
    if (!s5_found) {
        if (!parse_s5_from_dsdt()) {
            puts("ACPI: Cannot find _S5 sleep state");
            puts("ACPI: System halted (power off manually)");
            asm volatile("cli");
            for(;;) {
                asm volatile("hlt");
            }
        }
    }
    
    printf("ACPI: Entering S5 sleep state (shutdown)\n");
    printf("ACPI: PM1a_CNT=0x%x, PM1b_CNT=0x%x\n",
           g_fadt->pm1a_control_block, g_fadt->pm1b_control_block);
    
    // Disable interrupts
    asm volatile("cli");
    
    // Prepare sleep command for PM1a
    // Format: [SLP_EN=1][SLP_TYP=xxx][reserved]
    // Bit 13: SLP_EN (Sleep Enable)
    // Bits 10-12: SLP_TYP (Sleep Type from _S5)
    uint16_t pm1a_value = inw(g_fadt->pm1a_control_block);
    pm1a_value &= 0xE3FF;  // Clear SLP_TYP (bits 10-12) and SLP_EN (bit 13)
    pm1a_value |= (s5_slp_typa << 10);  // Set SLP_TYP
    pm1a_value |= (1 << 13);  // Set SLP_EN
    
    printf("ACPI: Writing 0x%x to PM1a_CNT\n", pm1a_value);
    outw(g_fadt->pm1a_control_block, pm1a_value);
    
    // If PM1b exists, write to it as well
    if (g_fadt->pm1b_control_block != 0) {
        uint16_t pm1b_value = inw(g_fadt->pm1b_control_block);
        pm1b_value &= 0xE3FF;
        pm1b_value |= (s5_slp_typb << 10);
        pm1b_value |= (1 << 13);
        
        printf("ACPI: Writing 0x%x to PM1b_CNT\n", pm1b_value);
        outw(g_fadt->pm1b_control_block, pm1b_value);
    }
    
    // System should power off now
    // If it doesn't, wait a bit and halt
    puts("ACPI: Waiting for shutdown...");
    for (volatile int i = 0; i < 10000000; i++);
    
    puts("ACPI: Shutdown failed, system halted");
    for(;;) {
        asm volatile("hlt");
    }
}

// Public API: Check if ACPI is available
bool acpi_is_available(void)
{
    return acpi_available;
}