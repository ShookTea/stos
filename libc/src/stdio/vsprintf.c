#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

int vsprintf(char* restrict buffer, const char* restrict format, va_list list)
{
    return vsnprintf(buffer, SIZE_MAX, format, list);
}
