.globl gdt_flush

gdt_flush:
    movl 4(%esp), %eax # Get the pointer to the GDT
    lgdt (%eax)        # Load the new GDT pointer

    mov $0x10, %ax     # 0x10 is the offset in the GDT to our data segment
    mov %ax, %ds       # Load all data segment selectors
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss
    ljmp $0x08, $.flush # 0x08 is the offset to our code segment

.flush:
    ret

.globl tss_flush
tss_flush:
    mov $0x28, %ax # TSS selector
    ltr %ax # Load Task Register
    ret
