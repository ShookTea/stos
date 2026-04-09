#include <kernel/page_fault.h>
#include <kernel/paging.h>
#include "kernel/debug.h"
#include <string.h>

// Global statistics
static page_fault_stats_t pf_stats;

// Custom handler (optional)
static page_fault_handler_t custom_handler = NULL;

// Stack guard region (typical stack grows down from here)
#define STACK_GUARD_REGION_START 0x00000000
#define STACK_GUARD_REGION_END   0x00100000

// Typical kernel stack location (adjust based on your layout)
#define KERNEL_STACK_START 0xC0000000
#define KERNEL_STACK_END   0xC0100000

/**
 * Analyze a page fault and fill in the info structure
 */
void page_fault_analyze(
    uint32_t faulting_addr,
    uint32_t error_code,
    uint32_t eip,
    uint32_t esp,
    uint32_t ebp,
    page_fault_info_t* info
)
{
    if (!info) {
        return;
    }

    // Clear the structure
    memset(info, 0, sizeof(page_fault_info_t));

    // Fill in basic information
    info->faulting_address = faulting_addr;
    info->error_code = error_code;
    info->eip = eip;
    info->esp = esp;
    info->ebp = ebp;

    // Decode error code
    info->present = (error_code & PF_PRESENT) != 0;
    info->write_access = (error_code & PF_WRITE) != 0;
    info->user_mode = (error_code & PF_USER) != 0;
    info->reserved_bit = (error_code & PF_RESERVED) != 0;
    info->instr_fetch = (error_code & PF_INSTR_FETCH) != 0;

    // Check if page is mapped
    info->page_mapped = paging_is_mapped(faulting_addr);
    if (info->page_mapped) {
        info->physical_address = paging_get_physical_address(faulting_addr);
    }

    // Categorize the fault
    if (page_fault_is_null_deref(faulting_addr)) {
        info->category = PF_CAT_NULL_DEREF;
        pf_stats.null_dereferences++;
    }
    else if (info->reserved_bit) {
        info->category = PF_CAT_RESERVED_BIT;
    }
    else if (info->present) {
        // Page is present, so this is a protection violation
        pf_stats.protection_violations++;

        if (info->write_access) {
            info->category = PF_CAT_WRITE_READONLY;
            pf_stats.write_readonly++;
        }
        else if (info->user_mode) {
            info->category = PF_CAT_USER_SUPERVISOR;
            pf_stats.user_supervisor++;
        }
        else if (info->instr_fetch) {
            info->category = PF_CAT_EXEC_NOEXEC;
            pf_stats.instruction_fetch++;
        }
        else {
            info->category = PF_CAT_PROTECTION;
        }
    }
    else {
        // Page not present
        pf_stats.not_present++;

        // Check if this might be a stack overflow
        if (faulting_addr < esp &&
            faulting_addr < STACK_GUARD_REGION_END) {
            info->category = PF_CAT_STACK_OVERFLOW;
        }
        else if (page_fault_is_kernel_addr(faulting_addr)) {
            info->category = PF_CAT_UNMAPPED_KERNEL;
        }
        else if (page_fault_is_user_addr(faulting_addr)) {
            info->category = PF_CAT_UNMAPPED_USER;
        }
        else {
            info->category = PF_CAT_NOT_PRESENT;
        }
    }

    pf_stats.total_faults++;
}

/**
 * Get category description string
 */
const char* page_fault_category_string(page_fault_category_t category)
{
    switch (category) {
        case PF_CAT_NULL_DEREF:
            return "NULL Pointer Dereference";
        case PF_CAT_NOT_PRESENT:
            return "Page Not Present";
        case PF_CAT_PROTECTION:
            return "Protection Violation";
        case PF_CAT_WRITE_READONLY:
            return "Write to Read-Only Page";
        case PF_CAT_USER_SUPERVISOR:
            return "User Mode Accessing Supervisor Page";
        case PF_CAT_RESERVED_BIT:
            return "Reserved Bit Set in Page Table";
        case PF_CAT_EXEC_NOEXEC:
            return "Execute Non-Executable Page";
        case PF_CAT_STACK_OVERFLOW:
            return "Stack Overflow";
        case PF_CAT_UNMAPPED_KERNEL:
            return "Unmapped Kernel Address";
        case PF_CAT_UNMAPPED_USER:
            return "Unmapped User Address";
        case PF_CAT_UNKNOWN:
        default:
            return "Unknown/Unclassified";
    }
}

/**
 * Print detailed page fault information
 */
void page_fault_print_info(const page_fault_info_t* info)
{
    if (!info) {
        return;
    }

    debug_printf("\n========================================\n");
    debug_printf("         PAGE FAULT EXCEPTION\n");
    debug_printf("========================================\n");

    debug_printf("\nFault Category: %s\n", page_fault_category_string(info->category));
    debug_printf("Faulting Address: %#010x\n", info->faulting_address);
    debug_printf("Error Code:       %#010x\n", info->error_code);

    // Detailed error code breakdown
    debug_printf("\nError Code Analysis:\n");
    debug_printf("  Present:          %s\n", info->present ? "YES (protection violation)" : "NO (page not present)");
    debug_printf("  Access Type:      %s\n", info->write_access ? "WRITE" : "READ");
    debug_printf("  Privilege Mode:   %s\n", info->user_mode ? "USER" : "SUPERVISOR");
    debug_printf("  Reserved Bit:     %s\n", info->reserved_bit ? "SET (invalid)" : "clear");
    debug_printf("  Instruction Fetch:%s\n", info->instr_fetch ? "YES" : "no");

    // Address analysis
    debug_printf("\nAddress Analysis:\n");
    #if KERNEL_DEBUG_ANY
        uint32_t page_aligned = paging_align_down(info->faulting_address);
        uint32_t offset = info->faulting_address & (PAGE_SIZE - 1);
    #endif
    debug_printf("  Page-aligned:     %#010x\n", page_aligned);
    debug_printf("  Page offset:      %#x (%u bytes)\n", offset, offset);
    debug_printf("  Page mapped:      %s\n", info->page_mapped ? "YES" : "NO");

    if (info->page_mapped) {
        debug_printf("  Physical address: %#010x\n", info->physical_address);
    }

    // Address region identification
    debug_printf("  Memory region:    ");
    if (info->faulting_address < 0x1000) {
        debug_printf("NULL page (0x0 - 0xFFF)\n");
    } else if (info->faulting_address < 0x100000) {
        debug_printf("Low memory (< 1MB)\n");
    } else if (info->faulting_address < 0x400000) {
        debug_printf("Kernel space (1MB - 4MB)\n");
    } else if (info->faulting_address >= 0x40000000 && info->faulting_address < 0xC0000000) {
        debug_printf("User space (1GB - 3GB)\n");
    } else if (info->faulting_address >= 0xC0000000) {
        debug_printf("Kernel space (> 3GB)\n");
    } else {
        debug_printf("Kernel heap/other\n");
    }

    // Instruction context
    debug_printf("\nInstruction Context:\n");
    debug_printf("  Faulting EIP:     %#010x", info->eip);
    if (paging_is_mapped(info->eip)) {
        debug_printf(" (mapped)\n");
    } else {
        debug_printf(" (NOT MAPPED - invalid!)\n");
    }

    // Stack context
    debug_printf("  Stack Pointer:    %#010x", info->esp);
    if (paging_is_mapped(info->esp)) {
        debug_printf(" (mapped)\n");
    } else {
        debug_printf(" (NOT MAPPED - corrupt!)\n");
    }

    debug_printf("  Base Pointer:     %#010x", info->ebp);
    if (paging_is_mapped(info->ebp)) {
        debug_printf(" (mapped)\n");
    } else {
        debug_printf(" (NOT MAPPED)\n");
    }

    // Specific fault explanations
    // Memory dump around faulting address (if safe to access)
    if (info->faulting_address >= 0x1000 &&
        info->faulting_address < 0xFFFF0000) {
        // Try to dump memory, but be careful
        debug_printf("\nMemory context around faulting address:\n");
        debug_printf("(Attempting to dump nearby memory...)\n");
        // Dump 64 bytes before and after
        page_fault_dump_memory(info->faulting_address, 64);
    }

    debug_printf("\nFault Explanation:\n");
    switch (info->category) {
        case PF_CAT_NULL_DEREF:
            debug_printf("  A NULL pointer (or near-NULL) was dereferenced.\n");
            debug_printf("  This is typically caused by:\n");
            debug_printf("    - Using an uninitialized pointer\n");
            debug_printf("    - Dereferencing a function that returned NULL\n");
            debug_printf("    - Array index out of bounds (negative)\n");
            break;

        case PF_CAT_WRITE_READONLY:
            debug_printf("  Attempted to write to a read-only page.\n");
            debug_printf("  This could be:\n");
            debug_printf("    - Writing to read-only data (.rodata section)\n");
            debug_printf("    - Writing to code (.text section)\n");
            debug_printf("    - Violating copy-on-write protection\n");
            break;

        case PF_CAT_USER_SUPERVISOR:
            debug_printf("  User mode code tried to access kernel memory.\n");
            debug_printf("  This is a security violation and indicates:\n");
            debug_printf("    - Buffer overflow exploit attempt\n");
            debug_printf("    - Incorrect privilege level\n");
            debug_printf("    - Corrupted user process state\n");
            break;

        case PF_CAT_NOT_PRESENT:
        case PF_CAT_UNMAPPED_KERNEL:
        case PF_CAT_UNMAPPED_USER:
            debug_printf("  Accessed memory that is not mapped.\n");
            debug_printf("  This could be:\n");
            debug_printf("    - Use-after-free (memory was freed)\n");
            debug_printf("    - Wild pointer (uninitialized or corrupted)\n");
            debug_printf("    - Missing memory allocation\n");
            debug_printf("    - Stack corruption\n");
            break;

        case PF_CAT_STACK_OVERFLOW:
            debug_printf("  Stack has grown beyond its allocated region.\n");
            debug_printf("  This is typically caused by:\n");
            debug_printf("    - Deep recursion\n");
            debug_printf("    - Large local variables/arrays\n");
            debug_printf("    - Infinite recursion\n");
            break;

        case PF_CAT_EXEC_NOEXEC:
            debug_printf("  Attempted to execute code from non-executable page.\n");
            debug_printf("  This could be:\n");
            debug_printf("    - Executing data as code\n");
            debug_printf("    - Stack-based exploit attempt (NX/DEP protection)\n");
            debug_printf("    - Corrupted function pointer\n");
            break;

        case PF_CAT_RESERVED_BIT:
            debug_printf("  A reserved bit was set in a page table entry.\n");
            debug_printf("  This indicates page table corruption!\n");
            break;

        default:
            debug_printf("  Unable to determine specific cause.\n");
            break;
    }
}

/**
 * Print stack trace
 */
void page_fault_print_stack_trace(uint32_t ebp, int max_frames)
{
    debug_printf("\nStack Trace:\n");

    if (!paging_is_mapped(ebp)) {
        debug_printf("  Cannot trace stack - EBP (%#x) not mapped\n", ebp);
        return;
    }

    uint32_t* frame = (uint32_t*)ebp;

    for (int i = 0; i < max_frames && frame; i++) {
        // Check if frame pointer is valid
        if (!paging_is_mapped((uint32_t)frame) ||
            !paging_is_mapped((uint32_t)frame + 4)) {
            debug_printf("  [%d] Frame pointer %#x not mapped (end of trace)\n", i, (uint32_t)frame);
            break;
        }

        uint32_t ret_addr = frame[1];
        debug_printf("  [%d] Return address: %#010x", i, ret_addr);

        if (paging_is_mapped(ret_addr)) {
            debug_printf(" (valid)\n");
        } else {
            debug_printf(" (INVALID - not mapped)\n");
            debug_printf("      Stack may be corrupted\n");
            break;
        }

        // Move to previous frame
        uint32_t next_ebp = frame[0];

        // Sanity checks
        if (next_ebp == 0) {
            debug_printf("  [End of stack - EBP is 0]\n");
            break;
        }

        if (next_ebp <= (uint32_t)frame) {
            debug_printf("  [%d] Invalid frame pointer %#x (not growing up)\n", i + 1, next_ebp);
            break;
        }

        if (next_ebp > (uint32_t)frame + 0x10000) {
            // Frame pointer jumped more than 64KB - suspicious
            debug_printf("  [%d] Suspicious frame pointer %#x (jumped %u bytes)\n",
                   i + 1, next_ebp, next_ebp - (uint32_t)frame);
            break;
        }

        frame = (uint32_t*)next_ebp;
    }
}

/**
 * Attempt to recover from page fault
 */
bool page_fault_try_recover(const page_fault_info_t* info)
{
    if (!info) {
        return false;
    }

    if (info->present && info->write_access) {
        // Check for copy-on-write page fault.
        if (paging_handle_page_fault_cow(info->faulting_address)) {
            pf_stats.recoverable++;
            debug_printf("COW page fault recovered at %#x\n", info->faulting_address);
            return true;
        }
    }

    // Try custom handler
    if (custom_handler) {
        if (custom_handler(info)) {
            pf_stats.recoverable++;
            return true;
        }
    }

    // Built-in recovery strategies could go here:
    // - Demand paging (allocate page on access)
    // - Stack expansion (grow stack automatically)
    // - Copy-on-write (allocate private copy)
    // - Swapping (swap in page from disk)

    // For now, we don't implement any automatic recovery
    // This would be added when implementing virtual memory features

    pf_stats.unrecoverable++;
    return false;
}

/**
 * Register custom handler
 */
void page_fault_register_handler(page_fault_handler_t handler)
{
    custom_handler = handler;
}

/**
 * Unregister custom handler
 */
void page_fault_unregister_handler(void)
{
    custom_handler = NULL;
}

/**
 * Get statistics
 */
void page_fault_get_stats(page_fault_stats_t* stats)
{
    if (!stats) {
        return;
    }

    *stats = pf_stats;
}

/**
 * Reset statistics
 */
void page_fault_reset_stats(void)
{
    memset(&pf_stats, 0, sizeof(page_fault_stats_t));
}

/**
 * Print statistics
 */
void page_fault_print_stats(void)
{
    debug_printf("\n=== Page Fault Statistics ===\n");
    debug_printf("Total page faults:      %u\n", pf_stats.total_faults);
    debug_printf("NULL dereferences:      %u\n", pf_stats.null_dereferences);
    debug_printf("Protection violations:  %u\n", pf_stats.protection_violations);
    debug_printf("  Write to read-only:   %u\n", pf_stats.write_readonly);
    debug_printf("  User->supervisor:     %u\n", pf_stats.user_supervisor);
    debug_printf("  Instruction fetch:    %u\n", pf_stats.instruction_fetch);
    debug_printf("Not present faults:     %u\n", pf_stats.not_present);
    debug_printf("Recoverable faults:     %u\n", pf_stats.recoverable);
    debug_printf("Unrecoverable faults:   %u\n", pf_stats.unrecoverable);

    if (pf_stats.total_faults > 0) {
        #if KERNEL_DEBUG_ANY
        uint32_t recovery_rate = (pf_stats.recoverable * 100) / pf_stats.total_faults;
        #endif
        debug_printf("Recovery rate:          %u%%\n", recovery_rate);
    }
}

/**
 * Dump memory around a faulting address for debugging
 *
 * @param addr Address to dump around
 * @param context Number of bytes before/after to dump
 */
void page_fault_dump_memory(uint32_t addr, uint32_t context)
{
    debug_printf("\nMemory Dump around %#010x:\n", addr);

    // Align to 16-byte boundary for nice formatting
    uint32_t start = (addr - context) & ~0xF;
    uint32_t end = (addr + context + 0xF) & ~0xF;

    debug_printf("Address   | +0 +1 +2 +3 +4 +5 +6 +7 +8 +9 +A +B +C +D +E +F | ASCII\n");
    debug_printf("----------+-------------------------------------------------+------------------\n");

    for (uint32_t line_addr = start; line_addr < end; line_addr += 16) {
        // Check if this line is mapped
        bool line_mapped = true;
        for (int i = 0; i < 16; i++) {
            if (!paging_is_mapped(line_addr + i)) {
                line_mapped = false;
                break;
            }
        }

        debug_printf("%08x | ", line_addr);

        if (line_mapped) {
            uint8_t* bytes = (uint8_t*)line_addr;

            // Print hex bytes
            for (int i = 0; i < 16; i++) {
                // Highlight the faulting byte
                if (line_addr + i == addr) {
                    debug_printf("\033[1;31m%02x\033[0m ", bytes[i]);  // Red highlight
                } else {
                    debug_printf("%02x ", bytes[i]);
                }
            }

            debug_printf("| ");

            // Print ASCII representation
            for (int i = 0; i < 16; i++) {
                uint8_t c = bytes[i];
                if (c >= 0x20 && c <= 0x7E) {
                    if (line_addr + i == addr) {
                        debug_printf("\033[1;31m%c\033[0m", c);  // Red highlight
                    } else {
                        debug_printf("%c", c);
                    }
                } else {
                    debug_printf(".");
                }
            }
            debug_printf("\n");
        } else {
            debug_printf("?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? | (unmapped)\n");
        }
    }

    debug_printf("----------+-------------------------------------------------+------------------\n");
    debug_printf("Legend: Faulting byte highlighted in red\n");
}
