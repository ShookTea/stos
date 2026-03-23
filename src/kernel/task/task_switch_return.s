# This is where switch_to_stack returns to for new tasks. At this point, we're
# on the new task's stack, and we need to "return" through the interrupt
# handler chain

.section .text
.align 4

.globl task_switch_return_point
.type task_switch_return_point, @function
task_switch_return_point:
    # We're now on the new task's stack
    # The stack has: segment regs, PUSHAL regs, int_no, err_code, IRET frame

    popal # Restore general purpose registers
    pop %ds # Restore segment registers
    pop %es
    pop %fs
    pop %gs
    addl $8, %esp # Skip int_no and err_code
    iret # Return to task entry point
