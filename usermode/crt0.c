extern int main(int argc, char** argv, char** envp);

#ifdef __cplusplus
// These are provided by the linker
extern void (*__init_array_start[])(void);
extern void (*__init_array_end[])(void);
extern void (*__fini_array_start[])(void);
extern void (*__fini_array_end[])(void);
#endif

/**
 * This file is an actual entrypoint for all usermode programs. The _start is
 * what's actually getting executed when running the app.
 *
 * It's responsible for things like:
 * - Initializing libc
 * - Setting up argc, argv, envp from stack
 * - Calling constructors
 * - Calling main()
 * - Calling exit()
 * - Calling destructors
 */
__attribute__((section(".text._start")))
void _start(void) {
    // 1. Initialize libc data structures (so far there's nothing needed)

    #ifdef __cplusplus
    // 2. Call global constructors (C++)
    int init_count = __init_array_end - __init_array_start;
    for (int i = 0; i < init_count; i++) {
        __init_array_start[i]();
    }
    #endif

    // 3. Parse arguments/env from stack
    // TODO: that's not implemented yet, empty values used instead
    int argc = 0;
    char** argv = 0;
    char** envp = 0;

    // 4. Call main
    int ret = main(argc, argv, envp);

    #ifdef __cplusplus
    // 5. Call global destructors (C++)
    int fini_count = __fini_array_end - __fini_array_start;
    for (int i = fini_count - 1; i >= 0; i--) {
        __fini_array_start[i]();
    }
    #endif

    // 6. Exit with status code

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

    __builtin_unreachable();
}
