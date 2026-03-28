#include <kernel/paging.h>
#include <kernel/memory/pmm.h>
#include <kernel/memory/vmm.h>
#include <kernel/multiboot2.h>
#include "paging.h"
#include <stdlib.h>
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

static size_t pages_saved_by_cow = 0;

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
        bool in_usermode = virt >= VMM_USER_START && virt < VMM_USER_END;
        pde->user = in_usermode ? 1 : 0;
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

static page_table_t* paging_get_page_table_from_pde(page_directory_entry_t* pde)
{
    if (!pde->present) {
        return NULL;
    }
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

void paging_switch_directory(void* page_dir_addr)
{
    page_directory_t* page_directory = (page_directory_t*)page_dir_addr;

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

void* paging_get_current_directory()
{
    return current_page_directory;
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

    printf(
        "Present Page Directory Entries: %u / %u\n",
        present_pdes,
        PAGE_DIRECTORY_SIZE
    );
    printf(
        "Total Mapped Pages: %u (%u KB, %u MB)\n",
        total_mapped_pages,
        total_mapped_pages * 4,
        (total_mapped_pages * 4) / 1024
    );
    printf(
        "Virtual Address Space Used: %.2f%%\n",
        (total_mapped_pages * 100.0) / (PAGE_DIRECTORY_SIZE * PAGE_TABLE_SIZE)
    );
    printf(
        "Pages saved by COW: %u\n",
        pages_saved_by_cow
    );
    puts("=========================");
}

void* paging_get_kernel_directory()
{
    return kernel_page_directory;
}

/**
 * Clone a page table for fork().
 * Strategy:
 * - For stack - allocate new physical pages and copy contents
 * - For data/heap - mark both parent and child as COW and read-only
 * - For shared kernel - should never appear in user space page tables
 */
static page_table_t* paging_clone_page_table(
    page_table_t* src_pt,
    uint32_t pd_index,
    uint32_t user_stack_base,
    uint32_t user_stack_size
) {
    if (src_pt == NULL) {
        return NULL;
    }

    // Allocate new page table
    uint32_t dst_pt_phys = pmm_alloc_page();
    if (dst_pt_phys == 0) {
        printf("PAGING: failed to allocate page table for cloning\n");
        abort(); // TODO: is abort() needed here?
        return NULL;
    }

    page_table_t* dst_pt = (page_table_t*)phys_to_virt_internal(dst_pt_phys);
    memset(dst_pt, 0, sizeof(page_table_t));

    // Calculate virt. addr. range for this page table
    uint32_t pt_virt_base = pd_index * (1024 * PAGE_SIZE);

    // Clone each entry
    for (uint32_t i = 0; i < PAGE_TABLE_SIZE; i++) {
        page_table_entry_t* src_pte = &src_pt->entries[i];
        page_table_entry_t* dst_pte = &dst_pt->entries[i];
        if (!src_pte->present) {
            // Skip non-present pages
            continue;
        }

        uint32_t virt = pt_virt_base + (i * PAGE_SIZE);
        // Determine if this is a stack page
        uint32_t stack_start = user_stack_base;
        uint32_t stack_end = user_stack_base + user_stack_size;
        bool is_stack_page = user_stack_size > 0
            && virt >= stack_start
            && virt < stack_end;

        if (is_stack_page) {
            // Stack pages are always physically copied
            uint32_t new_phys = pmm_alloc_page();
            if (dst_pt_phys == 0) {
                printf(
                    "Failed to allocate page table for stack clone at %#x\n",
                    virt
                );
                pmm_free_page(dst_pt_phys);
                // TODO: cleanup already allocated pages
                abort(); // TODO: is abort() needed here?
                return NULL;
            }

            // If source page exists, copy it, otherwise zero it
            if (src_pte->present) {
                uint32_t src_phys = src_pte->frame << 12;
                void* src_page = phys_to_virt_internal(src_phys);
                void* dst_page = phys_to_virt_internal(new_phys);
                memcpy(dst_page, src_page, PAGE_SIZE);
                *dst_pte = *src_pte;
                dst_pte->frame = new_phys >> 12;
            } else {
                void* dst_page = phys_to_virt_internal(new_phys);
                memset(dst_page, 0, PAGE_SIZE);
                // Set up PTE for usermode stack
                dst_pte->present = 1;
                dst_pte->rw = 1;
                dst_pte->user = 1;
                dst_pte->frame = new_phys >> 12;
            }

            // Ensure user flag is set for stack pages
            dst_pte->user = 1;
        } else if (src_pte->present) {
            // Data/heap pages use copy-on-write - mark as COW and read-only

            uint32_t phys = src_pte->frame << 12; // shared phys address
            uint16_t refcount = pmm_inc_refcount(phys);
            if (refcount == 0) {
                printf("Failed to increment refcount for page %#x\n", phys);
                // Cleanup and retrun NULL
                for (uint32_t j = 0; j < i; j++) {
                    if (dst_pt->entries[j].present) {
                        uint32_t cleanup_phys = dst_pt->entries[j].frame << 12;
                        bool is_cow = (dst_pt->entries[j].available & 1) != 0;
                        if (is_cow) {
                            pmm_dec_refcount(cleanup_phys);
                        } else {
                            pmm_free_page(cleanup_phys);
                        }
                    }
                }
                pmm_free_page(dst_pt_phys);
                abort(); // TODO: is abort needed here?
                return NULL;
            }

            *dst_pte = *src_pte;
            src_pte->rw = 0;
            src_pte->available |= (PAGE_COW >> 9);
            dst_pte->rw = 0;
            dst_pte->available |= (PAGE_COW >> 9);

            if (refcount == 2) {
                // For debugging. TODO: remove it.
                printf("PAGING: Page %#x now shared\n", phys);
            }
        }
        // else: paging is not present in source and not a stack page
        // leave as zero
    }

    return dst_pt;
}

void* paging_clone_directory(
    void* _src,
    bool usermode,
    uint32_t user_stack_base,
    uint32_t user_stack_size
) {
    page_directory_t* src = _src;
    if (src == NULL) {
        puts("PAGING: cannot clone NULL page directory");
        abort(); // TODO: is the abort() needed here?
        return NULL;
    }

    page_directory_t* dst = paging_create_directory();
    if (dst == NULL) {
        puts("PAGING: failed to clone page directory");
        abort(); // TODO: is the abort() needed here?
        return NULL;
    }

    // Copy entry 0 (identity mapping)
    dst->entries[0] = src->entries[0];

    // Copy physical memory mapping region
    uint32_t phys_map_start_pd = paging_get_pd_index(PHYS_MAP_BASE);
    uint32_t phys_map_end_pd = paging_get_pd_index(0xFFFFFFFF);
    for (uint32_t i = phys_map_start_pd; i <= phys_map_end_pd; i++) {
        if (src->entries[i].present) {
            dst->entries[i] = src->entries[i];
        }
    }

    // Copy kernel heap region
    uint32_t kernel_heap_start_pd = paging_get_pd_index(VMM_KERNEL_HEAP);
    uint32_t kernel_heap_end_pd = paging_get_pd_index(VMM_KERNEL_END);

    for (uint32_t i = kernel_heap_start_pd; i < kernel_heap_end_pd; i++) {
        if (src->entries[i].present) {
            dst->entries[i] = src->entries[i];
        }
    }

    // Handle user space
    if (usermode) {
        // Clone user space
        uint32_t user_start_pd = paging_get_pd_index(VMM_USER_START);
        uint32_t user_end_pd = paging_get_pd_index(VMM_USER_END);
        printf(
            "PAGING: Cloning user space (PD entries %u-%u)\n",
            user_start_pd,
            user_end_pd - 1
        );

        uint32_t cloned_tables = 0;
        for (uint32_t pd_idx = user_start_pd; pd_idx < user_end_pd; pd_idx++) {
            page_directory_entry_t* src_pde = &src->entries[pd_idx];
            if (!src_pde->present) {
                continue; // No page table at this index
            }
            page_table_t* src_pt = paging_get_page_table_from_pde(src_pde);
            if (src_pt == NULL) {
                printf(
                    "PAGING: Failed to access source page table at PD[%u]\n",
                    pd_idx
                );
                // Cleanup already cloned tables
                for (
                    uint32_t cleanup_idx = user_start_pd;
                    cleanup_idx < pd_idx;
                    cleanup_idx++
                ) {
                    if (dst->entries[cleanup_idx].present) {
                        page_table_t* cleanup_pt =
                            paging_get_page_table_from_pde(
                                &dst->entries[cleanup_idx]
                            );
                        if (cleanup_pt != NULL) {
                            // TODO: Free physical pages referenced by this page table
                            uint32_t cleanup_pt_phys =
                                dst->entries[cleanup_idx].frame << 12;
                            pmm_free_page(cleanup_pt_phys);
                        }
                    }
                }
                pmm_free_page(VIRT_TO_PHYS(dst));
                abort(); // TODO: is abort() needed here?
                return NULL;
            }

            page_table_t* dst_pt = paging_clone_page_table(
                src_pt,
                pd_idx,
                user_stack_base,
                user_stack_size
            );
            if (dst_pt == NULL) {
                printf("PAGING: Failed to clone PT at PD[%u]\n", pd_idx);
                // Cleanup already cloned tables
                for (
                    uint32_t cleanup_idx = user_start_pd;
                    cleanup_idx < pd_idx;
                    cleanup_idx++
                ) {
                    if (dst->entries[cleanup_idx].present) {
                        page_table_t* cleanup_pt =
                            paging_get_page_table_from_pde(
                                &dst->entries[cleanup_idx]
                            );
                        if (cleanup_pt != NULL) {
                            // TODO: Free physical pages referenced by this page table
                            uint32_t cleanup_pt_phys =
                                dst->entries[cleanup_idx].frame << 12;
                            pmm_free_page(cleanup_pt_phys);
                        }
                    }
                }
                pmm_free_page(VIRT_TO_PHYS(dst));
                abort(); // TODO: is abort needed here?
                return NULL;
            }

            uint32_t dst_pt_phys = VIRT_TO_PHYS(dst_pt);
            dst->entries[pd_idx] = *src_pde;
            dst->entries[pd_idx].frame = dst_pt_phys >> 12;
            cloned_tables++;
        }

        printf(
            "PAGING: Cloned %u userspace page tables\n",
            cloned_tables
        );
        // Flush TLB because we modified parent process's PTEs
        paging_flush_tlb();
    }

    printf(
        "PAGING: cloned page directory (kernel=%s, user=%s)\n",
        "yes",
        usermode ? "yes" : "no"
    );
    return dst;
}

bool paging_handle_page_fault_cow(uint32_t faulting_addr)
{
    uint32_t page_addr = paging_align_down(faulting_addr);
    uint32_t pd_index = page_addr >> 22;
    uint32_t pt_index = (page_addr >> 12) & 0x3FF;

    page_directory_t* current_pd = current_page_directory;
    page_directory_entry_t* pde = &current_pd->entries[pd_index];
    if (!pde->present) {
        return false; // Page table doesn't exist
    }
    uint32_t pt_phys = pde->frame << 12;
    page_table_t* pt = (page_table_t*)PHYS_TO_VIRT(pt_phys);
    page_table_entry_t* pte = &pt->entries[pt_index];
    if (!pte->present) {
        return false; // Page not present
    }
    if ((pte->available & 1) == 0) {
        return false; // Not a COW page (bit 9 in available field)
    }

    // This is a COW page - handle it
    printf("PAGING: COW fault at %#x (phys: %#x)\n",
           page_addr, pte->frame << 12);

    // Allocate new physical page
    uint32_t new_phys = pmm_alloc_page();
    if (new_phys == 0) {
        printf("PAGING: Failed to allocate page for COW at %#x\n", page_addr);
        return false;
    }

    // Copy contents from old page to new page
    uint32_t old_phys = pte->frame << 12;
    void* old_page = PHYS_TO_VIRT(old_phys);
    void* new_page = PHYS_TO_VIRT(new_phys);
    memcpy(new_page, old_page, PAGE_SIZE);

    pte->frame = new_phys >> 12; // Point to new physical page
    pte->rw = 1; // Make writable
    pte->available &= ~1; // Clear COW flag

    // Decrement reference count for old physical page
    // If reference count reaches 0, free the page
    uint16_t old_refcount = pmm_dec_refcount(old_phys);
    if (old_refcount == 0) {
        printf(
            "PAGING: old COW page %#x freed (last ref. removed)\n",
            old_phys
        );
    } else {
        printf(
            "PAGING: old COW page %#x still has %u references\n",
            old_phys,
            old_refcount
        );
    }

    // Flush TLB for this page
    paging_flush_tlb_entry(page_addr);

    printf("PAGING: COW resolved - new phys page %#x\n", new_phys);
    pages_saved_by_cow++;
    return true;
}

void paging_free_user_pages(void* _pd)
{
    page_directory_t* pd = (page_directory_t*)_pd;
    if (pd == NULL) {
        return;
    }

    uint32_t user_start_pd = paging_get_pd_index(VMM_USER_START);
    uint32_t user_end_pd = paging_get_pd_index(VMM_USER_END);
    uint32_t freed_tables = 0;
    uint32_t freed_pages = 0;

    for (uint32_t pd_idx = user_start_pd; pd_idx < user_end_pd; pd_idx++) {
        page_directory_entry_t* pde = &pd->entries[pd_idx];
        if (!pde->present) {
            continue;
        }

        page_table_t* pt = paging_get_page_table_from_pde(pde);
        if (pt == NULL) {
            continue;
        }

        // Free all pages referenced by this page table
        for (uint32_t pt_idx = 0; pt_idx < PAGE_TABLE_SIZE; pt_idx++) {
            page_table_entry_t* pte = &pt->entries[pt_idx];
            if (!pte->present) {
                continue;
            }

            // Check if this is a COW page (shared with other processes)
            bool is_cow = (pte->available & 1) != 0;

            if (is_cow) {
                // Decrement reference count
                uint32_t phys = pte->frame << 12;
                uint16_t new_refcount = pmm_dec_refcount(phys);
                if (new_refcount == 0) {
                    pmm_free_page(phys);
                    freed_pages++;
                }
                // else: page is still referenced by other processes
            } else {
                // This is a private page (e.g., stack page that was copied)
                // Safe to free
                uint32_t phys = pte->frame << 12;
                pmm_free_page(phys);
                freed_pages++;
            }
        }

        // Free the page table itself
        uint32_t pt_phys = pde->frame << 12;
        pmm_free_page(pt_phys);
        freed_tables++;
    }

    if (freed_tables > 0) {
        printf(
            "PAGING: Freed %u user page tables and %u private pages\n",
            freed_tables,
            freed_pages
        );
    }
}
