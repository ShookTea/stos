#include "kernel/debug.h"
#include "kernel/memory/kmalloc.h"
#include "kernel/serial.h"
#include <stdio.h>

#if (KERNEL_DEBUG_TERMINAL || KERNEL_DEBUG_COM)

int debug_puts(const char* string)
{
    #if KERNEL_DEBUG_COM && !KERNEL_DEBUG_TERMINAL
        int res = 0;
        for (;*string;string++) { serial_put_c(*string); res++; }
        serial_put_c('\n');
        return res + 1;
    #elif KERNEL_DEBUG_TERMINAL
        return puts(string);
    #else
        return 0;
    #endif
}

int debug_printf(const char* format, ...)
{
    #if KERNEL_DEBUG_COM && !KERNEL_DEBUG_TERMINAL
        int res = 0;
        char buffer[200];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, 200, format, args);
        va_end(args);
        char* string = buffer;
        for (;*string;string++) { serial_put_c(*string); res++; }
        return res;
    #elif KERNEL_DEBUG_TERMINAL
        va_list args;
        va_start(args, format);
        int res = vprintf(format, args);
        va_end(args);
        return res;
    #else
        return 0;
    #endif
}

int debug_putchar(int ic)
{
    #if KERNEL_DEBUG_COM && !KERNEL_DEBUG_TERMINAL
        serial_put_c(ic);
        return 1;
    #elif KERNEL_DEBUG_TERMINAL
        return putchar(ic);
    #else
        return 0;
    #endif
}

#endif
