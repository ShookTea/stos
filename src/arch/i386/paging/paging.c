#include <kernel/paging.h>
#include <kernel/memory/pmm.h>
#include <kernel/multiboot2.h>
#include "paging.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Assembly functions from paging_asm.s
extern void paging_load_directory(uint32_t* page_directory_physical);
extern void paging_enable(void);
extern void paging_disable(void);
extern void paging_flush_tlb_single(uint32_t virt_addr);
extern void paging_flush_tlb_all(void);
extern uint32_t paging_get_cr3(void);
extern uint32_t paging_get_cr2(void);
extern bool paging_is_enabled(void);

// Global kernel page directory (virtual address after paging enabled)
page_directory_t* kernel_page_directory = NULL;

// Current page directory (virtual address after paging enabled)
static page_directory_t* current_page_directory = NULL;

// Physical address of kernel page directory (needed for CR3)
static uint32_t kernel_page_directory_phys = 0;

// Track if we've completed paging initialization
static bool paging_fully_initialized = false;

/**
 * Convert physical address to virtual address for page table access
 * Before paging is fully set up, we use identity mapping (phys == virt)
 * After paging is set up, we use the PHYS_MAP_BASE region
 */
static inline void* phys_to_virt_internal(uint32_t phys)
{
    if (paging_fully_initialized) {
        // Use the physical memory mapping region
        return PHYS_TO_VIRT(phys);
    } else {
        // Before full initialization, use identity mapping
        return (void*)phys;
    }
}

/**
 * Get or create a page table for a given virtual address
 */
static page_table_t* paging_get_page_table(uint32_t virt, bool create)
{
    uint32_t pd_index = paging_get_pd_index(virt);
    page_directory_entry_t* pde = &current_page_directory->entries[pd_index];

    // Check if page table exists
    if (!pde->present) {
        if (!create) {
            return NULL;
        }

        // Allocate a new page table
        uint32_t pt_phys = pmm_alloc_page();
        if (pt_phys == 0) {
            printf("PAGING: Failed to allocate page table\n");
            return NULL;
        }

        // Convert physical address to virtual for access
        page_table_t* pt = (page_table_t*)phys_to_virt_internal(pt_phys);

        // Clear the page table
        memset(pt, 0, sizeof(page_table_t));

        // Set up the page directory entry (always uses physical address)
        pde->present = 1;
        pde->rw = 1;
        pde->user = 0;  // Kernel page table
        pde->frame = pt_phys >> 12;

        // Flush TLB for this virtual address range
        if (paging_is_enabled()) {
            paging_flush_tlb_entry(virt);
        }

        return pt;
    }

    // Return existing page table
    uint32_t pt_phys = pde->frame << 12;
    page_table_t* pt = (page_table_t*)phys_to_virt_internal(pt_phys);

    return pt;
}

void paging_init(void)
{
    printf("Initializing paging...\n");

    // Check if paging is already enabled
    if (paging_is_enabled()) {
        printf("WARNING: Paging is already enabled!\n");
        return;
    }

    // Allocate the kernel page directory (returns physical address)
    uint32_t pd_phys = pmm_alloc_page();
    if (pd_phys == 0) {
        printf("PAGING: Failed to allocate page directory\n");
        return;
    }

    kernel_page_directory_phys = pd_phys;

    // Before paging is enabled, physical == virtual, so we can access it directly
    kernel_page_directory = (page_directory_t*)pd_phys;
    current_page_directory = kernel_page_directory;

    // Clear the page directory
    memset(kernel_page_directory, 0, sizeof(page_directory_t));

    printf("Kernel page directory at: %#x (physical)\n", pd_phys);

    // Identity map the first 4MB (kernel space)
    // This covers:
    // - 0x00000000 - 0x000FFFFF : Reserved (BIOS, VGA)
    // - 0x00100000 - 0x003FFFFF : Kernel code, data, PMM bitmap, etc.
    printf("Identity mapping first 4MB...\n");

    uint32_t identity_map_end = 0x400000;  // 4MB
    uint32_t pages_mapped = 0;

    for (uint32_t addr = 0; addr < identity_map_end; addr += PAGE_SIZE) {
        if (!paging_map_page(addr, addr, PAGE_FLAGS_KERNEL)) {
            printf("PAGING: Failed to map page at %#x\n", addr);
            return;
        }
        pages_mapped++;
    }

    printf("Identity mapped %u pages (0x0 - %#x)\n",
           pages_mapped, identity_map_end - 1);

    // Map physical memory to PHYS_MAP_BASE for kernel access
    // This allows us to access any physical address safely after paging is enabled
    printf("Mapping physical memory to %#x...\n", PHYS_MAP_BASE);

    // We'll map up to 1GB of physical RAM (or whatever PMM tells us exists)
    uint32_t phys_mem_size = pmm_get_total_memory();
    
    // Check if boot modules extend beyond reported memory size
    uint32_t max_module_addr = 0;
    uint32_t module_count = multiboot2_get_modules_count();
    
    if (module_count > 0) {
        printf("Checking %u boot module(s) for memory mapping:\n", module_count);
        for (uint32_t i = 0; i < module_count; i++) {
            multiboot_tag_boot_module_t* module = multiboot2_get_boot_module_entry(i);
            if (module) {
                printf("  Module %u: %#x - %#x\n", i, 
                       module->module_phys_addr_start,
                       module->module_phys_addr_end);
                if (module->module_phys_addr_end > max_module_addr) {
                    max_module_addr = module->module_phys_addr_end;
                }
            }
        }
        printf("Max module address: %#x\n", max_module_addr);
    }
    
    // Ensure we map at least up to the last boot module
    if (max_module_addr > phys_mem_size) {
        // Align max_module_addr up to page boundary
        max_module_addr = paging_align_up(max_module_addr);
        printf("Boot modules extend beyond reported memory (%#x > %#x)\n",
               max_module_addr, phys_mem_size);
        printf("Adjusting mapping size from %#x to %#x\n", 
               phys_mem_size, max_module_addr);
        phys_mem_size = max_module_addr;
    }
    
    if (phys_mem_size > PHYS_MAP_SIZE) {
        printf("WARNING: Physical memory (%u MB) exceeds mapping size (%u MB)\n",
               phys_mem_size / (1024 * 1024),
               PHYS_MAP_SIZE / (1024 * 1024));
        phys_mem_size = PHYS_MAP_SIZE;
    }

    uint32_t phys_map_pages = 0;
    for (uint32_t phys = 0; phys < phys_mem_size; phys += PAGE_SIZE) {
        uint32_t virt = PHYS_MAP_BASE + phys;
        if (!paging_map_page(virt, phys, PAGE_FLAGS_KERNEL)) {
            printf("PAGING: Failed to map physical page %#x to virtual %#x\n", phys, virt);
            return;
        }
        phys_map_pages++;
    }

    printf("Mapped %u pages of physical memory (%u MB)\n",
           phys_map_pages, (phys_map_pages * 4096) / (1024 * 1024));

    // Load the page directory into CR3 (use physical address)
    paging_load_directory((uint32_t*)pd_phys);
    printf("Page directory loaded into CR3\n");

    // Enable paging
    paging_enable();
    printf("Paging enabled!\n");

    // Now that paging is enabled AND physical memory is mapped,
    // update our pointers to use virtual addresses from the PHYS_MAP_BASE region
    kernel_page_directory = (page_directory_t*)PHYS_TO_VIRT(pd_phys);
    current_page_directory = kernel_page_directory;

    // Mark that we're fully initialized
    paging_fully_initialized = true;

    printf("Switched to virtual addressing (PD at %#x)\n", (uint32_t)kernel_page_directory);

    // Verify paging is enabled
    if (paging_is_enabled()) {
        printf("Paging status: ENABLED\n");
    } else {
        printf("ERROR: Paging failed to enable!\n");
    }
}

bool paging_map_page(uint32_t virt, uint32_t phys, uint32_t flags)
{
    // Align addresses
    virt = paging_align_down(virt);
    phys = paging_align_down(phys);

    // Get the page table (create if needed)
    page_table_t* pt = paging_get_page_table(virt, true);
    if (pt == NULL) {
        return false;
    }

    // Get the page table entry
    uint32_t pt_index = paging_get_pt_index(virt);
    page_table_entry_t* pte = &pt->entries[pt_index];

    // Check if already mapped
    if (pte->present) {
        // For now, allow remapping (needed during initialization)
        // In production, you might want to return false or explicitly unmap first
        if (paging_fully_initialized) {
            printf("PAGING: Warning - page %#x already mapped to %#x, remapping to %#x\n",
                   virt, pte->frame << 12, phys);
        }
    }

    // Set up the page table entry
    pte->present = (flags & PAGE_PRESENT) ? 1 : 0;
    pte->rw = (flags & PAGE_WRITE) ? 1 : 0;
    pte->user = (flags & PAGE_USER) ? 1 : 0;
    pte->pwt = (flags & PAGE_WRITETHROUGH) ? 1 : 0;
    pte->pcd = (flags & PAGE_CACHE_DISABLE) ? 1 : 0;
    pte->global = (flags & PAGE_GLOBAL) ? 1 : 0;
    pte->frame = phys >> 12;

    // Flush TLB entry
    if (paging_is_enabled()) {
        paging_flush_tlb_entry(virt);
    }

    return true;
}

void paging_unmap_page(uint32_t virt)
{
    // Align address
    virt = paging_align_down(virt);

    // Get the page table
    page_table_t* pt = paging_get_page_table(virt, false);
    if (pt == NULL) {
        return;  // Not mapped
    }

    // Get the page table entry
    uint32_t pt_index = paging_get_pt_index(virt);
    page_table_entry_t* pte = &pt->entries[pt_index];

    // Clear the entry
    if (pte->present) {
        memset(pte, 0, sizeof(page_table_entry_t));
        if (paging_is_enabled()) {
            paging_flush_tlb_entry(virt);
        }
    }
}

uint32_t paging_get_physical_address(uint32_t virt)
{
    // Get the page table
    page_table_t* pt = paging_get_page_table(virt, false);
    if (pt == NULL) {
        return 0;  // Not mapped
    }

    // Get the page table entry
    uint32_t pt_index = paging_get_pt_index(virt);
    page_table_entry_t* pte = &pt->entries[pt_index];

    if (!pte->present) {
        return 0;  // Not mapped
    }

    // Return physical address
    uint32_t phys_page = pte->frame << 12;
    uint32_t offset = paging_get_page_offset(virt);
    return phys_page | offset;
}

bool paging_is_mapped(uint32_t virt)
{
    // Get the page table
    page_table_t* pt = paging_get_page_table(virt, false);
    if (pt == NULL) {
        return false;
    }

    // Get the page table entry
    uint32_t pt_index = paging_get_pt_index(virt);
    page_table_entry_t* pte = &pt->entries[pt_index];

    return pte->present;
}

void paging_switch_directory(page_directory_t* page_directory)
{
    if (page_directory == NULL) {
        printf("PAGING: Cannot switch to NULL page directory\n");
        return;
    }

    current_page_directory = page_directory;

    // Convert virtual address to physical for CR3
    uint32_t pd_phys;
    if (paging_fully_initialized && IS_PHYS_MAPPED(page_directory)) {
        pd_phys = VIRT_TO_PHYS(page_directory);
    } else {
        // During initialization or if using identity mapping
        pd_phys = (uint32_t)page_directory;
    }

    paging_load_directory((uint32_t*)pd_phys);
}

void paging_flush_tlb_entry(uint32_t virt)
{
    paging_flush_tlb_single(virt);
}

void paging_flush_tlb(void)
{
    paging_flush_tlb_all();
}

page_directory_t* paging_create_directory(void)
{
    // Allocate a new page directory (returns physical address)
    uint32_t pd_phys = pmm_alloc_page();
    if (pd_phys == 0) {
        printf("PAGING: Failed to allocate page directory\n");
        return NULL;
    }

    // Convert to virtual address for access
    page_directory_t* pd = (page_directory_t*)phys_to_virt_internal(pd_phys);
    memset(pd, 0, sizeof(page_directory_t));

    return pd;
}

page_directory_t* paging_clone_kernel_directory(void)
{
    if (kernel_page_directory == NULL) {
        printf("PAGING: Kernel page directory not initialized\n");
        return NULL;
    }

    // Create a new page directory
    page_directory_t* pd = paging_create_directory();
    if (pd == NULL) {
        return NULL;
    }

    // Copy kernel page directory entries:
    // 1. Identity map region (first 4MB = entry 0)
    pd->entries[0] = kernel_page_directory->entries[0];

    // 2. Physical memory mapping region (PHYS_MAP_BASE area)
    uint32_t phys_map_start_pd = paging_get_pd_index(PHYS_MAP_BASE);
    uint32_t phys_map_end_pd = paging_get_pd_index(0xFFFFFFFF);

    for (uint32_t i = phys_map_start_pd; i <= phys_map_end_pd; i++) {
        if (kernel_page_directory->entries[i].present) {
            pd->entries[i] = kernel_page_directory->entries[i];
        }
    }

    printf("PAGING: Cloned kernel directory (identity map + phys map)\n");

    return pd;
}

/**
 * Debug: Dump page directory information
 */
void paging_dump_page_directory(void)
{
    if (current_page_directory == NULL) {
        printf("No page directory loaded\n");
        return;
    }

    printf("=== Page Directory Dump ===\n");
    printf("Page Directory Virtual Address: %#x\n", (uint32_t)current_page_directory);
    printf("CR3 (Physical): %#x\n", paging_get_cr3());
    printf("Paging Enabled: %s\n", paging_is_enabled() ? "Yes" : "No");
    printf("\nPresent Page Directory Entries:\n");

    uint32_t present_count = 0;
    for (uint32_t i = 0; i < PAGE_DIRECTORY_SIZE; i++) {
        page_directory_entry_t* pde = &current_page_directory->entries[i];
        if (pde->present) {
            printf("  PDE[%u]: phys=%#x, %s, %s\n",
                   i,
                   pde->frame << 12,
                   pde->rw ? "RW" : "RO",
                   pde->user ? "USER" : "KERNEL");
            present_count++;
        }
    }

    printf("Total present PDEs: %u / %u\n", present_count, PAGE_DIRECTORY_SIZE);
    printf("===========================\n");
}

/**
 * Debug: Dump a specific page table
 */
void paging_dump_page_table(uint32_t pd_index)
{
    if (current_page_directory == NULL) {
        printf("No page directory loaded\n");
        return;
    }

    if (pd_index >= PAGE_DIRECTORY_SIZE) {
        printf("Invalid page directory index: %u\n", pd_index);
        return;
    }

    page_directory_entry_t* pde = &current_page_directory->entries[pd_index];
    if (!pde->present) {
        printf("Page directory entry %u is not present\n", pd_index);
        return;
    }

    uint32_t pt_phys = pde->frame << 12;
    page_table_t* pt = (page_table_t*)phys_to_virt_internal(pt_phys);

    printf("=== Page Table Dump (PD Index %u) ===\n", pd_index);
    printf("Page Table Physical Address: %#x\n", pt_phys);
    printf("Page Table Virtual Address: %#x\n", (uint32_t)pt);
    printf("Virtual Address Range: %#x - %#x\n",
           pd_index * 4 * 1024 * 1024,
           (pd_index + 1) * 4 * 1024 * 1024 - 1);
    printf("\nPresent Page Table Entries:\n");

    uint32_t present_count = 0;
    for (uint32_t i = 0; i < PAGE_TABLE_SIZE; i++) {
        page_table_entry_t* pte = &pt->entries[i];
        if (pte->present) {
            uint32_t virt = (pd_index << 22) | (i << 12);
            printf("  PTE[%u]: virt=%#x -> phys=%#x, %s, %s%s%s\n",
                   i,
                   virt,
                   pte->frame << 12,
                   pte->rw ? "RW" : "RO",
                   pte->user ? "USER" : "KERNEL",
                   pte->accessed ? ", ACCESSED" : "",
                   pte->dirty ? ", DIRTY" : "");
            present_count++;

            // Limit output to avoid spamming
            if (present_count >= 20) {
                uint32_t remaining = 0;
                for (uint32_t j = i + 1; j < PAGE_TABLE_SIZE; j++) {
                    if (pt->entries[j].present) remaining++;
                }
                if (remaining > 0) {
                    printf("  ... (%u more entries)\n", remaining);
                }
                break;
            }
        }
    }

    if (present_count <= 20) {
        printf("Total present PTEs: %u / %u\n", present_count, PAGE_TABLE_SIZE);
    }
    printf("======================================\n");
}

/**
 * Validate that critical memory regions are properly identity-mapped
 */
bool paging_validate_identity_mapping(void)
{
    printf("=== Validating Identity Mapping ===\n");

    bool all_valid = true;

    // Test critical addresses
    struct {
        uint32_t addr;
        const char* description;
    } test_addresses[] = {
        { 0x00000000, "Zero page (BIOS)" },
        { 0x000B8000, "VGA text buffer" },
        { 0x00100000, "Kernel start (1MB)" },
        { 0x00200000, "Kernel middle (2MB)" },
        { 0x003FF000, "End of identity map (4MB - 1 page)" },
    };

    for (size_t i = 0; i < sizeof(test_addresses) / sizeof(test_addresses[0]); i++) {
        uint32_t virt = test_addresses[i].addr;
        uint32_t phys = paging_get_physical_address(virt);
        bool mapped = paging_is_mapped(virt);
        bool identity = (phys == virt);

        printf("  %#010x (%s): %s%s\n",
               virt,
               test_addresses[i].description,
               mapped ? (identity ? "MAPPED (identity)" : "MAPPED (not identity!)") : "NOT MAPPED!",
               mapped && !identity ? " ERROR!" : "");

        if (!mapped || !identity) {
            all_valid = false;
        }
    }

    if (all_valid) {
        printf("Result: All critical regions are identity-mapped correctly\n");
    } else {
        printf("Result: VALIDATION FAILED - some regions are not properly mapped!\n");
    }

    printf("===================================\n");
    return all_valid;
}

/**
 * Print statistics about current paging state
 */
void paging_print_stats(void)
{
    if (!paging_is_enabled()) {
        printf("Paging is not enabled\n");
        return;
    }

    printf("=== Paging Statistics ===\n");
    printf("Paging Status: ENABLED\n");
    printf("Current CR3 (Physical): %#x\n", paging_get_cr3());
    printf("Kernel Page Directory (Virtual): %#x\n", (uint32_t)kernel_page_directory);
    printf("Current Page Directory (Virtual): %#x\n", (uint32_t)current_page_directory);
    printf("Physical Memory Mapping: %#x - %#x\n", PHYS_MAP_BASE, 0xFFFFFFFF);

    // Count present page directory entries
    uint32_t present_pdes = 0;
    uint32_t total_mapped_pages = 0;

    for (uint32_t i = 0; i < PAGE_DIRECTORY_SIZE; i++) {
        page_directory_entry_t* pde = &current_page_directory->entries[i];
        if (pde->present) {
            present_pdes++;

            // Count pages in this page table
            uint32_t pt_phys = pde->frame << 12;
            page_table_t* pt = (page_table_t*)phys_to_virt_internal(pt_phys);

            for (uint32_t j = 0; j < PAGE_TABLE_SIZE; j++) {
                if (pt->entries[j].present) {
                    total_mapped_pages++;
                }
            }
        }
    }

    printf("Present Page Directory Entries: %u / %u\n", present_pdes, PAGE_DIRECTORY_SIZE);
    printf("Total Mapped Pages: %u (%u KB, %u MB)\n",
           total_mapped_pages,
           total_mapped_pages * 4,
           (total_mapped_pages * 4) / 1024);
    printf("Virtual Address Space Used: %.2f%%\n",
           (total_mapped_pages * 100.0) / (PAGE_DIRECTORY_SIZE * PAGE_TABLE_SIZE));
    printf("=========================\n");
}
