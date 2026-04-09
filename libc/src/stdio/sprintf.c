#include <limits.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>

int sprintf(char* restrict buffer, const char* restrict format, ...)
{
    va_list parameters;
    va_start(parameters, format);
    int written = vsnprintf(buffer, SIZE_MAX, format, parameters);
    va_end(parameters);
    return written;
}
