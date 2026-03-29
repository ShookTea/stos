.section .text
.align 4

# void switch_to_stack(
#     uint32_t* old_esp_ptr,
#     uint32_t new_esp
# )
.globl switch_to_stack
.type switch_to_stack, @function
switch_to_stack:
    # Parameters:
    # [esp + 4] = old_esp_ptr (pointer to where to save old ESP)
    # [esp + 8] = new_esp (new stack pointer)

    push %ebp
    mov %esp, %ebp

    # Save callee-saved registers (C calling convention)
    push %ebx
    push %esi
    push %edi

    # Get parameters
    mov 8(%ebp), %eax       # old_esp_ptr
    mov 12(%ebp), %edx      # new_esp

    # Save current ESP to old task (if not NULL)
    test %eax, %eax
    jz .no_save
    mov %esp, (%eax)        # Save current stack pointer

.no_save:
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
