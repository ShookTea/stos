#include <stdlib.h>
#include <stdint.h>

extern int main(int argc, char** argv, char** envp);
extern char** environ;

#ifdef __cplusplus
// These are provided by the linker
extern void (*__init_array_start[])(void);
extern void (*__init_array_end[])(void);
extern void (*__fini_array_start[])(void);
extern void (*__fini_array_end[])(void);
#endif

/**
 * C entry point called by _start with the raw entry-time stack pointer.
 *
 * At process entry the System V i386 ABI places on the stack:
 *   sp[0]           = argc
 *   sp[1..argc]     = argv[0..argc-1]
 *   sp[argc+1]      = NULL  (argv sentinel)
 *   sp[argc+2..]    = envp[0], envp[1], ..., NULL
 */
__attribute__((used))
static void _crt_start(uint32_t* sp) {
    int argc = (int)sp[0];
    char** argv = (char**)&sp[1];
    char** envp = (char**)&sp[argc + 2];
    environ = envp;

    #ifdef __cplusplus
    // Call global constructors (C++)
    int init_count = __init_array_end - __init_array_start;
    for (int i = 0; i < init_count; i++) {
        __init_array_start[i]();
    }
    #endif

    int ret = main(argc, argv, envp);

    #ifdef __cplusplus
    // Call global destructors (C++)
    int fini_count = __fini_array_end - __fini_array_start;
    for (int i = fini_count - 1; i >= 0; i--) {
        __fini_array_start[i]();
    }
    #endif

    exit(ret);
    __builtin_unreachable();
}

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
 *
 * The function is naked so the compiler emits no prologue/epilogue; ESP at
 * entry still points directly at argc as set up by the kernel.  We pass ESP
 * as a pointer to the C helper _crt_start().
 */
__attribute__((naked))
__attribute__((section(".text._start")))
void _start(void) {
    __asm__ volatile (
        /* ESP currently points at argc.  Pass it as the sole argument to
         * _crt_start, then call it.  _crt_start never returns. */
        "push %%esp\n\t"
        "call _crt_start\n\t"
        /* Should never reach here. */
        "ud2\n\t"
        :
        :
        : "memory"
    );
}
