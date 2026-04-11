#ifndef KERNEL_DEBUG_H
#define KERNEL_DEBUG_H

/**
 * If set to 1 - enable sending debug messages to the terminal
 */
#define KERNEL_DEBUG_TERMINAL 0

/**
 * If set to 1 - enable sending debug messages to serial port. Needs to be
 * compared with KERNEL_DEBUG_TERMINAL - when terminal debugging is enabled,
 * there's a separate check for "KERNEL_DEBUG_COM".
 */
#define KERNEL_DEBUG_COM 1

#define KERNEL_DEBUG_ANY (KERNEL_DEBUG_TERMINAL || KERNEL_DEBUG_COM)


#if (KERNEL_DEBUG_ANY)
    void debug_puts(const char* string);
    void debug_puts_c(const char* cat, const char* string);
    void debug_printf(const char* format, ...);
    void debug_printf_c(const char* cat, const char* format, ...);
    void debug_newline();
#else
    // No-op definitions
    #define debug_puts(...) ((void)0)
    #define debug_puts_c(...) ((void)0)
    #define debug_printf(...) ((void)0)
    #define debug_printf_c(...) ((void)0)
    #define debug_newline(...) ((void)0)
#endif

#endif
