#include <limits.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>

/**
 * Format placeholder is in a following syntax:
 * %[flags][width][.precision][length]type
 *
 * Type can be any of:
 * - s - null-terminated string
 *
 * [flags] can be zero or more (in any order) of:
 * - `-` - left-align the output of placeholder (the default is right-align)
 * - `+` - prepend a plus for positive signed-numeric types (by default it only
 *         prepends minus for negative numbers)
 * - ` ` - prepend a space for positive signed-numeric types
 * - `0` - uses zeros for padding (by default it uses spaces)
 */
int vprintf(const char* restrict format, va_list list)
{
    int chars = 0;

    for (int i = 0; format[i]; ++i) {
        if (format[i] != '%') {
            // It's not a formatted value - just print it
            chars += putchar(format[i]);
            continue;
        }
        i++;

        // Reading flag block
        bool leftAlign = false;
        bool plusPrepend = false;
        bool spacePrepend = false;
        bool zeroPad = false;
        bool flagBreak = false;
        while (!flagBreak) {
            switch (format[i]) {
                case '-':
                    leftAlign = true;
                    i++;
                    break;
                case '+':
                    plusPrepend = true;
                    i++;
                    break;
                case ' ':
                    spacePrepend = true;
                    i++;
                    break;
                case '0':
                    zeroPad = true;
                    i++;
                    break;
                default:
                    flagBreak = true;
                    break;
            }
        }

        // Reading specified width
        int width = 0;
        while (isdigit(format[i])) {
            width *= 10;
            width += format[i] - 48;
            i++;
        }
    }

    return chars;
}
