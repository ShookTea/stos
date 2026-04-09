#include <limits.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <stddef.h>

int snprintf(char* restrict buffer, size_t n, const char* restrict format, ...)
{
    va_list parameters;
    va_start(parameters, format);
    int written = vsnprintf(buffer, n, format, parameters);
    va_end(parameters);
    return written;
}
