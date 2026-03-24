# usermode_enter(void* entry, void* stack)
.globl usermode_enter
usermode_enter:
    # Disable interrupts while we manipulate the stack
    cli

    # Get parameters from the stack
    movl 4(%esp), %ecx   # entry point
    movl 8(%esp), %edx   # user stack pointer

    # Set up segment registers for user mode
    mov $0x23, %ax       # User data segment (GDT entry 4, RPL=3)
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    # Build IRET frame on the current (kernel) stack
    # The IRET instruction expects (from bottom to top):
    # - EIP (instruction pointer)
    # - CS (code segment)
    # - EFLAGS
    # - ESP (stack pointer) - only when changing privilege level
    # - SS (stack segment) - only when changing privilege level

    pushl $0x23          # SS (user data segment)
    pushl %edx           # ESP (user stack)
    pushfl               # Push current EFLAGS
    popl %eax            # Pop into EAX to modify
    orl $0x200, %eax     # Set IF (bit 9) to enable interrupts
    pushl %eax           # Push modified EFLAGS
    pushl $0x1B          # CS (user code segment, GDT entry 3, RPL=3)
    pushl %ecx           # EIP (entry point)

    # Execute IRET to switch to user mode
    # This will:
    # - Load CS:EIP from the stack (jumps to entry point)
    # - Load EFLAGS from the stack
    # - Load SS:ESP from the stack (switches to user stack)
    # - Switch from ring 0 to ring 3
    iret
