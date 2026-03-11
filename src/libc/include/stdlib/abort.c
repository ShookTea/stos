#include <stdlib.h>
#include <stdio.h>

#if defined(__is_libk)
#include <kernel/tty.h>
#endif

__attribute__((__noreturn__))
void abort(void)
{
    #if defined(__is_libk)
        // Disable interrupts to prevent any further execution
        asm volatile("cli");

        // Set red background with white text for panic message
        tty_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_RED));

        // Display panic message
        tty_writestring("\n\n");
        tty_writestring("================================================================================\n");
        tty_writestring("                            KERNEL PANIC: abort()                               \n");
        tty_writestring("================================================================================\n");
        tty_writestring("\n");
        tty_writestring("The kernel has encountered a fatal error and cannot continue.\n");
        tty_writestring("System halted.\n");

        // Halt the CPU in an infinite loop
        while(1) {
            asm volatile("hlt");
        }
    #else
        // TODO: Abnormally terminate the process as if by SIGABRT.
        puts("abort()");
    #endif
    while(1) {}
    __builtin_unreachable();
}
