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
    // TODO: syscall to exit with ret value
    while(1); // hang for now
}
