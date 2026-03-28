extern int main(void);

/**
 * This file is an actual entrypoint for all usermode programs. The _start is
 * what's actually getting executed when running the app.
 *
 * It's responsible for things like:
 * - Setting up argc, argv, envp from stack
 * - Initializing libc
 * - Calling constructors
 * - Calling main()
 * - Calling exit()
 * - Calling destructors
 */
__attribute__((section(".text._start")))
void _start(void) {
    int ret = main();

    // Syscall: exit(ret)
    // Syscall number 1 (SYS_EXIT) in EAX, status code in ECX
    __asm__ volatile(
        "movl $1, %%eax\n" // 1 = SYS_EXIT
        "movl %0, %%ecx\n" // status code in ECX (arg1)
        "int $0x80\n"      // trigger syscall
        :                  // no outputs
        : "r"(ret)         // input: ret value
        : "eax", "ecx"     // clobbered registers
    );

    while(1); // hang for now
}
