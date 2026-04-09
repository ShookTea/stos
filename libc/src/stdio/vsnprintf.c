#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include "./_stdio_format.h"

static int emit_char(__stdio_format_out_t* out, char c)
{
    if (out->output.str.len + 1 < out->output.str.cap) {
        out->output.str.s[out->output.str.len] = c;
        out->output.str.len++;
        return 1;
    }

    return 0;
}

int vsnprintf(
    char* restrict buffer,
    size_t n,
    const char* restrict format,
    va_list list
) {
    __stdio_format_out_t out;
    out.emit_char = emit_char;
    out.output.str.s = buffer;
    out.output.str.len = 0;
    out.output.str.cap = n;
    int res = __stdio_format_core(&out, format, list);
    buffer[out.output.str.len] = '\0';
    return res;
}
