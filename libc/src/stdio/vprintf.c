#include <stdarg.h>
#include "./_stdio_format.h"
#include <stdio.h>

static int emit_char(__stdio_format_out_t* out __attribute__((unused)), char c)
{
    return putchar(c) == EOF ? 0 : 1;
}

int vprintf(const char* restrict format, va_list list)
{
    __stdio_format_out_t out;
    out.emit_char = emit_char;
    return __stdio_format_core(&out, format, list);
}
