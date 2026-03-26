# x86 Paging Assembly Functions
# These functions handle low-level paging operations that must be done
# in assembly

.section .text
.align 4

# void paging_load_directory(uint32_t* page_directory_physical)
# Load a page directory address into CR3
.global paging_load_directory
.type paging_load_directory, @function
paging_load_directory:
    push %ebp
    mov %esp, %ebp

    # Get the page directory address from the first argument
    mov 8(%ebp), %eax

    # Load it into CR3
    mov %eax, %cr3

    pop %ebp
    ret
.size paging_load_directory, . - paging_load_directory

# void paging_enable(void)
# Enable paging by setting the PG bit in CR0
.global paging_enable
.type paging_enable, @function
paging_enable:
    push %ebp
    mov %esp, %ebp

    # Read CR0
    mov %cr0, %eax

    # Set the paging bit (bit 31)
    or $0x80010000, %eax

    # Write back to CR0
    mov %eax, %cr0

    pop %ebp
    ret
.size paging_enable, . - paging_enable

# void paging_disable(void)
# Disable paging by clearing the PG bit in CR0
.global paging_disable
.type paging_disable, @function
paging_disable:
    push %ebp
    mov %esp, %ebp

    # Read CR0
    mov %cr0, %eax

    # Clear the paging bit (bit 31)
    and $0x7FFFFFFF, %eax

    # Write back to CR0
    mov %eax, %cr0

    pop %ebp
    ret
.size paging_disable, . - paging_disable

# void paging_flush_tlb_single(uint32_t virt_addr)
# Flush a single TLB entry for the given virtual address
.global paging_flush_tlb_single
.type paging_flush_tlb_single, @function
paging_flush_tlb_single:
    push %ebp
    mov %esp, %ebp

    # Get the virtual address from the first argument
    mov 8(%ebp), %eax

    # Invalidate the TLB entry for this address
    invlpg (%eax)

    pop %ebp
    ret
.size paging_flush_tlb_single, . - paging_flush_tlb_single

# void paging_flush_tlb_all(void)
# Flush the entire TLB by reloading CR3
.global paging_flush_tlb_all
.type paging_flush_tlb_all, @function
paging_flush_tlb_all:
    push %ebp
    mov %esp, %ebp

    # Read CR3
    mov %cr3, %eax

    # Write it back (this flushes the TLB)
    mov %eax, %cr3

    pop %ebp
    ret
.size paging_flush_tlb_all, . - paging_flush_tlb_all

# uint32_t paging_get_cr3(void)
# Get the current page directory address from CR3
.global paging_get_cr3
.type paging_get_cr3, @function
paging_get_cr3:
    push %ebp
    mov %esp, %ebp

    # Read CR3 into EAX (return value)
    mov %cr3, %eax

    pop %ebp
    ret
.size paging_get_cr3, . - paging_get_cr3

# uint32_t paging_get_cr2(void)
# Get the faulting address from CR2 (used in page fault handler)
.global paging_get_cr2
.type paging_get_cr2, @function
paging_get_cr2:
    push %ebp
    mov %esp, %ebp

    # Read CR2 into EAX (return value)
    mov %cr2, %eax

    pop %ebp
    ret
.size paging_get_cr2, . - paging_get_cr2

# bool paging_is_enabled(void)
# Check if paging is currently enabled
# Returns 1 if enabled, 0 if disabled
.global paging_is_enabled
.type paging_is_enabled, @function
paging_is_enabled:
    push %ebp
    mov %esp, %ebp

    # Read CR0
    mov %cr0, %eax

    # Test bit 31 (PG bit)
    shr $31, %eax
    and $1, %eax

    pop %ebp
    ret
.size paging_is_enabled, . - paging_is_enabled
