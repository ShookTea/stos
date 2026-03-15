#ifndef KERNEL_PAGE_FAULT_H
#define KERNEL_PAGE_FAULT_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Page Fault Handler
 * 
 * Provides detailed analysis and diagnostics for page faults.
 * This module integrates with the paging subsystem to provide
 * meaningful error messages and recovery options.
 */

// Page fault error code flags (from CPU)
#define PF_PRESENT      0x01  // 0: not present, 1: protection violation
#define PF_WRITE        0x02  // 0: read, 1: write
#define PF_USER         0x04  // 0: supervisor, 1: user mode
#define PF_RESERVED     0x08  // 1: reserved bit set in page table
#define PF_INSTR_FETCH  0x10  // 1: instruction fetch

// Page fault categories (for analysis)
typedef enum {
    PF_CAT_NULL_DEREF,          // NULL pointer dereference
    PF_CAT_NOT_PRESENT,         // Page not mapped
    PF_CAT_PROTECTION,          // Protection violation
    PF_CAT_WRITE_READONLY,      // Write to read-only page
    PF_CAT_USER_SUPERVISOR,     // User accessing supervisor page
    PF_CAT_RESERVED_BIT,        // Reserved bit set
    PF_CAT_EXEC_NOEXEC,         // Execute non-executable page
    PF_CAT_STACK_OVERFLOW,      // Stack overflow (below stack limit)
    PF_CAT_UNMAPPED_KERNEL,     // Unmapped kernel address
    PF_CAT_UNMAPPED_USER,       // Unmapped user address
    PF_CAT_UNKNOWN              // Unknown/unclassified
} page_fault_category_t;

// Page fault context information
typedef struct {
    uint32_t faulting_address;   // Address that caused the fault (CR2)
    uint32_t error_code;          // CPU error code
    uint32_t eip;                 // Instruction pointer at fault
    uint32_t esp;                 // Stack pointer at fault
    uint32_t ebp;                 // Base pointer at fault
    
    // Analysis results
    bool present;                 // Page was present
    bool write_access;            // Was a write operation
    bool user_mode;               // Faulting code in user mode
    bool reserved_bit;            // Reserved bit violation
    bool instr_fetch;             // Instruction fetch
    bool page_mapped;             // Is the page mapped?
    uint32_t physical_address;    // Physical address (if mapped)
    
    page_fault_category_t category;  // Fault category
} page_fault_info_t;

/**
 * Analyze a page fault and categorize it
 * 
 * @param faulting_addr Address from CR2
 * @param error_code CPU error code
 * @param eip Instruction pointer
 * @param esp Stack pointer
 * @param ebp Base pointer
 * @param info Output structure with analysis results
 */
void page_fault_analyze(
    uint32_t faulting_addr,
    uint32_t error_code,
    uint32_t eip,
    uint32_t esp,
    uint32_t ebp,
    page_fault_info_t* info
);

/**
 * Print detailed page fault information
 * 
 * @param info Page fault information structure
 */
void page_fault_print_info(const page_fault_info_t* info);

/**
 * Get a human-readable description of a page fault category
 * 
 * @param category Page fault category
 * @return String description
 */
const char* page_fault_category_string(page_fault_category_t category);

/**
 * Attempt to recover from a page fault
 * 
 * This function can implement:
 * - Demand paging (allocate page on access)
 * - Copy-on-write
 * - Stack expansion
 * - Swapping (future)
 * 
 * @param info Page fault information
 * @return true if recovery successful, false if unrecoverable
 */
bool page_fault_try_recover(const page_fault_info_t* info);

/**
 * Print a stack trace starting from given EBP
 * 
 * @param ebp Base pointer to start trace
 * @param max_frames Maximum number of frames to print
 */
void page_fault_print_stack_trace(uint32_t ebp, int max_frames);

/**
 * Check if an address is likely a NULL pointer dereference
 * 
 * @param addr Address to check
 * @return true if likely NULL dereference
 */
static inline bool page_fault_is_null_deref(uint32_t addr)
{
    // Consider addresses < 4KB as NULL dereferences
    return addr < 0x1000;
}

/**
 * Check if an address is in kernel space
 * 
 * @param addr Address to check
 * @return true if in kernel space
 */
static inline bool page_fault_is_kernel_addr(uint32_t addr)
{
    return addr >= 0xC0000000;
}

/**
 * Check if an address is in user space
 * 
 * @param addr Address to check
 * @return true if in user space
 */
static inline bool page_fault_is_user_addr(uint32_t addr)
{
    return addr >= 0x40000000 && addr < 0xC0000000;
}

/**
 * Register a custom page fault handler
 * 
 * Allows other subsystems to hook into page fault handling
 * for custom behavior (e.g., demand paging)
 * 
 * @param handler Handler function pointer
 */
typedef bool (*page_fault_handler_t)(const page_fault_info_t* info);
void page_fault_register_handler(page_fault_handler_t handler);

/**
 * Unregister custom page fault handler
 */
void page_fault_unregister_handler(void);

/**
 * Get statistics about page faults
 */
typedef struct {
    uint32_t total_faults;           // Total page faults
    uint32_t null_dereferences;      // NULL pointer dereferences
    uint32_t protection_violations;  // Protection violations
    uint32_t not_present;            // Not present faults
    uint32_t write_readonly;         // Write to read-only
    uint32_t user_supervisor;        // User->supervisor access
    uint32_t instruction_fetch;      // Instruction fetch faults
    uint32_t recoverable;            // Successfully recovered
    uint32_t unrecoverable;          // Could not recover
} page_fault_stats_t;

/**
 * Get page fault statistics
 * 
 * @param stats Output statistics structure
 */
void page_fault_get_stats(page_fault_stats_t* stats);

/**
 * Reset page fault statistics
 */
void page_fault_reset_stats(void);

/**
 * Print page fault statistics
 */
void page_fault_print_stats(void);

/**
 * Dump memory around a faulting address for debugging
 * 
 * Displays a hex dump of memory surrounding the faulting address,
 * with the faulting byte highlighted. Useful for debugging data
 * corruption, off-by-one errors, and buffer overflows.
 * 
 * @param addr Address to dump around
 * @param context Number of bytes before/after to dump
 */
void page_fault_dump_memory(uint32_t addr, uint32_t context);

#endif // KERNEL_PAGE_FAULT_H