.macro isr_err_stub num
isr_stub_\num:
    pushl $\num # push interrupt number
    jmp common_stub
.endm

.macro isr_no_err_stub num
isr_stub_\num:
    pushl $0 # push dummy error code
    pushl $\num # push interrupt number
    jmp common_stub
.endm

common_stub:
    pushal          # Save all GP registers
    cld
    movl $0x10, %eax  # Kernel data segment (adjust if needed)
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movl %esp, %eax  # Pointer to stack frame
    pushl %eax      # Pass frame ptr to C handler
    call exception_handler
    addl $4, %esp   # Pop frame ptr
    popal
    addl $8, %esp   # Skip int_no and err_code
    iret

.extern exception_handler
isr_no_err_stub 0  # Divide error
isr_no_err_stub 1  # Debug exception
isr_no_err_stub 2  # NMI interrupt
isr_no_err_stub 3  # Breakpoint
isr_no_err_stub 4  # Overflow
isr_no_err_stub 5  # BOUND range exceeded
isr_no_err_stub 6  # Invalid opcode
isr_no_err_stub 7  # Device not available (no math coprocessor)
isr_err_stub 8     # Double fault
isr_no_err_stub 9  # coprocessor segment overrun
isr_err_stub 10    # invalid TTS
isr_err_stub 11    # Segment not present
isr_err_stub 12    # Stack-Segment fault
isr_err_stub 13    # General protection
isr_err_stub 14    # Page fault
isr_no_err_stub 15 # Intel-reserved
isr_no_err_stub 16 # FPU math fault
isr_err_stub 17    # Alignment check
isr_no_err_stub 18 # Machine check
isr_no_err_stub 19 # SIMD floating point exception
isr_no_err_stub 20 # Virtualization exception
isr_no_err_stub 21 # Control protection exception
isr_no_err_stub 22 # from this one to 31: reserved for future use
isr_no_err_stub 23
isr_no_err_stub 24
isr_no_err_stub 25
isr_no_err_stub 26
isr_no_err_stub 27
isr_no_err_stub 28
isr_no_err_stub 29
isr_err_stub 30
isr_no_err_stub 31

# IRQ interrupts:
isr_no_err_stub 32 # IRQ 0 (Programmable Interrupt Timer)
isr_no_err_stub 33 # IRQ 1 (Keyboard)
isr_no_err_stub 34 # IRQ 2 (Cascade from slave PIC)
isr_no_err_stub 35 # IRQ 3 (COM2)
isr_no_err_stub 36 # IRQ 4 (COM1)
isr_no_err_stub 37 # IRQ 5 (LPT2)
isr_no_err_stub 38 # IRQ 6 (Floppy disk)
isr_no_err_stub 39 # IRQ 7 (LPT1)
isr_no_err_stub 40 # IRQ 8 (CMOS real-time clock)
isr_no_err_stub 41 # IRQ 9 (Free for peripherals / legacy SCSI / NIC)
isr_no_err_stub 42 # IRQ 10 (Free for peripherals / SCSI / NIC)
isr_no_err_stub 43 # IRQ 11 (Free for peripherals / SCSI / NIC)
isr_no_err_stub 44 # IRQ 12 PS2 mouse
isr_no_err_stub 45 # IRQ 13 FPU / coprocessor / inter-processor
isr_no_err_stub 46 # IRQ 14 Primary ATA hard disk
isr_no_err_stub 47 # IRQ 15 Secondary ATA hard disk

.globl isr_stub_table
isr_stub_table:
    .long isr_stub_0
    .long isr_stub_1
    .long isr_stub_2
    .long isr_stub_3
    .long isr_stub_4
    .long isr_stub_5
    .long isr_stub_6
    .long isr_stub_7
    .long isr_stub_8
    .long isr_stub_9
    .long isr_stub_10
    .long isr_stub_11
    .long isr_stub_12
    .long isr_stub_13
    .long isr_stub_14
    .long isr_stub_15
    .long isr_stub_16
    .long isr_stub_17
    .long isr_stub_18
    .long isr_stub_19
    .long isr_stub_20
    .long isr_stub_21
    .long isr_stub_22
    .long isr_stub_23
    .long isr_stub_24
    .long isr_stub_25
    .long isr_stub_26
    .long isr_stub_27
    .long isr_stub_28
    .long isr_stub_29
    .long isr_stub_30
    .long isr_stub_31
    .long isr_stub_32
    .long isr_stub_33
    .long isr_stub_34
    .long isr_stub_35
    .long isr_stub_36
    .long isr_stub_37
    .long isr_stub_38
    .long isr_stub_39
    .long isr_stub_40
    .long isr_stub_41
    .long isr_stub_42
    .long isr_stub_43
    .long isr_stub_44
    .long isr_stub_45
    .long isr_stub_46
    .long isr_stub_47
