# Minimal context switch - just swap ESP and CR3
# Everything else is handled by interrupt mechanism

.section .text
.align 4

# void switch_to_stack(
#     uint32_t* old_esp_ptr,
#     uint32_t new_esp,
#     uint32_t new_cr3
# )
.globl switch_to_stack
.type switch_to_stack, @function
switch_to_stack:
    # Parameters:
    # [esp + 4] = old_esp_ptr (pointer to where to save old ESP)
    # [esp + 8] = new_esp (new stack pointer)
    # [esp + 12] = new_cr3 (new page directory)

    push %ebp
    mov %esp, %ebp

    # Save callee-saved registers (C calling convention)
    push %ebx
    push %esi
    push %edi

    # Get parameters
    mov 8(%ebp), %eax       # old_esp_ptr
    mov 12(%ebp), %edx      # new_esp
    mov 16(%ebp), %ecx      # new_cr3

    # Save current ESP to old task (if not NULL)
    test %eax, %eax
    jz .no_save
    mov %esp, (%eax)        # Save current stack pointer

.no_save:
    # Switch page directory BEFORE switching stack
    # (important: do this while still on old stack)
    mov %ecx, %cr3

    # Switch to new stack
    mov %edx, %esp
    mov %edx, %ebp

    # Restore callee-saved registers from new stack
    pop %edi
    pop %esi
    pop %ebx
    pop %ebp

    # Return (now on new stack, in new address space!)
    ret
