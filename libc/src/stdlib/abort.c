#include <stdlib.h>
#include <stdio.h>

#if defined(__is_libk)
    #include <kernel/terminal.h>
#endif

__attribute__((__noreturn__))
void abort(void)
{
    #if defined(__is_libk)
        // Disable interrupts to prevent any further execution
        asm volatile("cli");

        puts("");

        // Set red background with white text for panic message,
        // then clear screen from the cursor forward
        printf("\033[0;41;37m\033[0J");

        // Display panic message
        puts("");
        puts("=============================================================");
        puts("                    KERNEL PANIC: abort()                    ");
        puts("=============================================================");
        puts("");
        puts("The kernel has encountered a fatal error and cannot continue.");
        puts("System halted.");

        // Halt the CPU in an infinite loop
        while(1) {
            asm volatile("hlt");
        }
    #else
        // TODO: Abnormally terminate the process as if by SIGABRT.
        puts("abort()");
        exit(1);

    #endif
    while(1) {}
    __builtin_unreachable();
}
