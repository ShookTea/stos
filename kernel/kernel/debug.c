#include "kernel/debug.h"
#include "kernel/serial.h"
#include <stdio.h>
#include <string.h>

#if (KERNEL_DEBUG_ANY)

static inline int cat_color(const char* cat)
{
    int i = 0;
    char* t = (char*)cat;
    while (*t != 0) {
        i += *t;
        t++;
    }
    return (i % 8) + 30;
}

void debug_puts(const char* string)
{
    #if KERNEL_DEBUG_COM && !KERNEL_DEBUG_TERMINAL
        for (;*string;string++) serial_put_c(*string);
        serial_put_c('\n');
    #elif KERNEL_DEBUG_TERMINAL
        puts(string);
    #endif
}

void debug_puts_c(const char* cat, const char* string)
{
    debug_printf("\033[%d;1m[%s]\033[0m %s\n", cat_color(cat), cat, string);
}

void debug_printf(const char* format, ...)
{
    #if KERNEL_DEBUG_COM && !KERNEL_DEBUG_TERMINAL
        char buffer[200];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, 200, format, args);
        va_end(args);
        char* string = buffer;
        for (;*string;string++) serial_put_c(*string);
    #elif KERNEL_DEBUG_TERMINAL
        va_list args;
        va_start(args, format);
        int res = vprintf(format, args);
        va_end(args);
    #endif
}

void debug_printf_c(const char* cat, const char* format, ...)
{
    #if KERNEL_DEBUG_COM && !KERNEL_DEBUG_TERMINAL
        char buffer[200];
        snprintf(buffer, 200, "\033[%d;1m[%s]\033[0m ", cat_color(cat), cat);
        char* string = buffer;
        for (;*string;string++) serial_put_c(*string);
        memset(buffer, 0, 200);

        va_list args;
        va_start(args, format);
        vsnprintf(buffer, 200, format, args);
        va_end(args);
        string = buffer;
        for (;*string;string++) serial_put_c(*string);
    #elif KERNEL_DEBUG_TERMINAL
        printf("\033[%d;1m[%s]\033[0m ", cat_color(cat), cat);
        va_list args;
        va_start(args, format);
        int res = vprintf(format, args);
        va_end(args);
    #endif
}

#endif
