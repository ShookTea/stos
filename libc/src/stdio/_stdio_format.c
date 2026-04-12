#include <limits.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include "./_stdio_format.h"

#define EMIT(c) do { if (!out->emit_char(out, (c))) return chars; chars++; } while (0)

static char* __int_to_str(
    intmax_t i,
    char b[],
    uint8_t base,
    bool prependPlus,
    bool prependSpace,
    uint8_t minWidth,
    bool alignLeft,
    bool zeroPad
) {
    char digit[32] = {0};
    memset(digit, 0, 32);
    strcpy(digit, "0123456789");
    if (base == 16) {
        strcat(digit, "abcdef");
    } else if (base == 17) {
        strcat(digit, "ABCDEF");
        base = 16;
    }

    char* p = b;
    if (i < 0) {
        *p++ = '-';
        i *= -1;
    } else if (prependPlus) {
        *p++ = '+';
    } else if (!prependPlus && prependSpace) {
        *p++ = ' ';
    }
    intmax_t shifter = i;
    do {
        ++p;
        shifter = shifter / base;
    } while (shifter);

    *p = '\0';
    do {
        *--p = digit[i % base];
        i = i / base;
    } while (i);

    int padding = minWidth - (int)strlen(b);
    if (padding < 0) padding = 0;

    if (alignLeft) {
        while (padding--) {
            b[strlen(b)] = zeroPad ? '0' : ' ';
        }
    } else {
        char a[256] = {0};
        while (padding--) {
            a[strlen(a)] = zeroPad ? '0' : ' ';
        }
        strcat(a, b);
        strcpy(b, a);
    }
    return b;
}

static bool emit_str(__stdio_format_out_t* out, int* chars, const char* s)
{
    for (; *s; s++) {
        if (!out->emit_char(out, *s)) return false;
        (*chars)++;
    }
    return true;
}

/**
 * Format placeholder is in a following syntax:
 * %[flags][width][.precision][size]type
 *
 * Type can be any of:
 * - d, i - signed integer. They are synonymous
 * - u - unsigned integer
 * - f, F - double in a fixed-point notation
 * - e, E - double in exponential notation
 * - x, X - unsigned integer as a hex number
 * - o - unsigned integer in octal
 * - s - null-terminated string
 * - c - `char`
 * - p - `void*` (pointer to void)
 * - n - print nothing, but write number of characters successfully written so
 *       far into an integer pointer parameter
 * - % - print a literal '%' character (doesn't accept any flags)
 *
 * [flags] can be zero or more (in any order) of:
 * - `-` - left-align the output of placeholder (the default is right-align)
 * - `+` - prepend a plus for positive signed-numeric types (by default it only
 *         prepends minus for negative numbers)
 * - ` ` - prepend a space for positive signed-numeric types
 * - `0` - uses zeros for padding (by default it uses spaces)
 * - `#` - enables alternate form:
 *         - for hex and octal modes (`o`, `x`, `X`) prepends `0`, `0x` and `0x`
 *
 * [width] specifies a minimum number of characters to output, and uses padding
 * to fill the remainder (as configured in [flags])
 *
 * [precision] usually specifies a maximum number of characters, depending on
 * the format:
 * - for floating point numeric types, it specifies the number of digits to
 *   the right of the decimal point and rounds the number
 * - for string type it limits the number of characters and the string is
 *   truncated after that limit
 *
 * [size], if defined, defines size of the argument:
 * - "hh" - `int` sized argument promoted from `char`
 * - "h" - `int` sized argument promoted from `short`
 * - "l" - `long` sized argument
 * - "ll" - `long long` sized argument
 * - "L" - `long double` argument
 * - "z" - `size_t`
 * - "j" - `intmax_t`
 * - "t" - `ptrdiff_t`
 */
int __stdio_format_core(
    __stdio_format_out_t* out,
    const char* format,
    va_list list
) {
    int chars = 0;
    char intStrBuffer[256] = {0};

    for (int i = 0; format[i]; ++i) {
        if (format[i] != '%') {
            // It's not a formatted value - just print it
            EMIT(format[i]);
            continue;
        }
        i++;

        // Reading flag block
        bool leftAlign = false;
        bool plusPrepend = false;
        bool spacePrepend = false;
        bool zeroPad = false;
        bool altMode = false;
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
                case '#':
                    altMode = true;
                    i++;
                    break;
                default:
                    flagBreak = true;
                    break;
            }
        }

        // Reading specified width
        uint8_t width = 0;
        while (isdigit(format[i])) {
            width *= 10;
            width += format[i] - '0';
            i++;
        }

        // Reading specified precision
        uint8_t precision = 0;
        if (format[i] == '.') {
            i++;
            while (isdigit(format[i])) {
                precision *= 10;
                precision += format[i] - '0';
                i++;
            }
        } else {
            precision = 6;
        }

        // Reading specified size mode
        char size = '\0';
        if (format[i] == 'h' || format[i] == 'l' || format[i] == 'j' ||
            format[i] == 'z' || format[i] == 't' || format[i] == 'L') {
            size = format[i];
            i++;
            if (format[i] == 'h') {
                size = 'H';
                i++;
            } else if (format[i] == 'l') {
                size = 'q';
                i++;
            }
        }

        char typeSpecifier = format[i];
        uint8_t numberBase = 10;
        memset(intStrBuffer, 0, 256);

        // convert specifier %o into %u (unsigned int) with different base
        if (typeSpecifier == 'o') {
            numberBase = 8;
            typeSpecifier = 'u';
            if (altMode) {
                EMIT('0');
            }
        }
        // convert specifier %p into %u (unsigned int) with different base
        if (typeSpecifier == 'p') {
            numberBase = 16;
            size = 'z';
            typeSpecifier = 'u';
        }

        // convert specifier %x into %u (unsigned int) with different base
        if (typeSpecifier == 'x') {
            numberBase = 16;
            typeSpecifier = 'u';
            if (altMode) {
                if (!emit_str(out, &chars, "0x")) return chars;
            }
        }
        // convert specifier %x into %u (unsigned int) with different base
        if (typeSpecifier == 'X') {
            // use a "fake" base-17 as "base-16, but uppercase"
            numberBase = 17;
            typeSpecifier = 'u';
            if (altMode) {
                if (!emit_str(out, &chars, "0X")) return chars;
            }
        }

        // Printing unsigned integers
        if (typeSpecifier == 'u') {
            switch (size) {
                case 0:
                {
                    unsigned int integer = va_arg(list, unsigned int);
                    __int_to_str(
                        integer, intStrBuffer, numberBase, plusPrepend,
                        spacePrepend, width, leftAlign, zeroPad
                    );
                    if (!emit_str(out, &chars, intStrBuffer)) return chars;
                    break;
                }
                case 'H':
                {
                    unsigned char integer = va_arg(list, unsigned int);
                    __int_to_str(
                        integer, intStrBuffer, numberBase, plusPrepend,
                        spacePrepend, width, leftAlign, zeroPad
                    );
                    if (!emit_str(out, &chars, intStrBuffer)) return chars;
                    break;
                }
                case 'h':
                {
                    unsigned short int integer = va_arg(list, unsigned int);
                    __int_to_str(
                        integer, intStrBuffer, numberBase, plusPrepend,
                        spacePrepend, width, leftAlign, zeroPad
                    );
                    if (!emit_str(out, &chars, intStrBuffer)) return chars;
                    break;
                }
                case 'l':
                {
                    unsigned long integer = va_arg(list, unsigned long);
                    __int_to_str(
                        integer, intStrBuffer, numberBase, plusPrepend,
                        spacePrepend, width, leftAlign, zeroPad
                    );
                    if (!emit_str(out, &chars, intStrBuffer)) return chars;
                    break;
                }
                case 'q':
                {
                    unsigned long long integer = va_arg(list, unsigned long long);
                    __int_to_str(
                        integer, intStrBuffer, numberBase, plusPrepend,
                        spacePrepend, width, leftAlign, zeroPad
                    );
                    if (!emit_str(out, &chars, intStrBuffer)) return chars;
                    break;
                }
                case 'j':
                {
                    uintmax_t integer = va_arg(list, uintmax_t);
                    __int_to_str(
                        integer, intStrBuffer, numberBase, plusPrepend,
                        spacePrepend, width, leftAlign, zeroPad
                    );
                    if (!emit_str(out, &chars, intStrBuffer)) return chars;
                    break;
                }
                case 'z':
                {
                    size_t integer = va_arg(list, size_t);
                    __int_to_str(
                        integer, intStrBuffer, numberBase, plusPrepend,
                        spacePrepend, width, leftAlign, zeroPad
                    );
                    if (!emit_str(out, &chars, intStrBuffer)) return chars;
                    break;
                }
                case 't':
                {
                    ptrdiff_t integer = va_arg(list, ptrdiff_t);
                    __int_to_str(
                        integer, intStrBuffer, numberBase, plusPrepend,
                        spacePrepend, width, leftAlign, zeroPad
                    );
                    if (!emit_str(out, &chars, intStrBuffer)) return chars;
                    break;
                }
                default:
                    break;
            }
        }

        // Printing signed integers
        if (typeSpecifier == 'd' || typeSpecifier == 'i') {
            switch (size) {
                case 0:
                {
                    int integer = va_arg(list, int);
                    __int_to_str(
                        integer, intStrBuffer, numberBase, plusPrepend,
                        spacePrepend, width, leftAlign, zeroPad
                    );
                    if (!emit_str(out, &chars, intStrBuffer)) return chars;
                    break;
                }
                case 'H':
                {
                    signed char integer = va_arg(list, int);
                    __int_to_str(
                        integer, intStrBuffer, numberBase, plusPrepend,
                        spacePrepend, width, leftAlign, zeroPad
                    );
                    if (!emit_str(out, &chars, intStrBuffer)) return chars;
                    break;
                }
                case 'h':
                {
                    short int integer = va_arg(list, int);
                    __int_to_str(
                        integer, intStrBuffer, numberBase, plusPrepend,
                        spacePrepend, width, leftAlign, zeroPad
                    );
                    if (!emit_str(out, &chars, intStrBuffer)) return chars;
                    break;
                }
                case 'l':
                {
                    long integer = va_arg(list, long);
                    __int_to_str(
                        integer, intStrBuffer, numberBase, plusPrepend,
                        spacePrepend, width, leftAlign, zeroPad
                    );
                    if (!emit_str(out, &chars, intStrBuffer)) return chars;
                    break;
                }
                case 'q':
                {
                    long long integer = va_arg(list, long long);
                    __int_to_str(
                        integer, intStrBuffer, numberBase, plusPrepend,
                        spacePrepend, width, leftAlign, zeroPad
                    );
                    if (!emit_str(out, &chars, intStrBuffer)) return chars;
                    break;
                }
                case 'j':
                {
                    intmax_t integer = va_arg(list, intmax_t);
                    __int_to_str(
                        integer, intStrBuffer, numberBase, plusPrepend,
                        spacePrepend, width, leftAlign, zeroPad
                    );
                    if (!emit_str(out, &chars, intStrBuffer)) return chars;
                    break;
                }
                case 'z':
                {
                    size_t integer = va_arg(list, size_t);
                    __int_to_str(
                        integer, intStrBuffer, numberBase, plusPrepend,
                        spacePrepend, width, leftAlign, zeroPad
                    );
                    if (!emit_str(out, &chars, intStrBuffer)) return chars;
                    break;
                }
                case 't':
                {
                    ptrdiff_t integer = va_arg(list, ptrdiff_t);
                    __int_to_str(
                        integer, intStrBuffer, numberBase, plusPrepend,
                        spacePrepend, width, leftAlign, zeroPad
                    );
                    if (!emit_str(out, &chars, intStrBuffer)) return chars;
                    break;
                }
                default:
                    break;
            }
        }

        // printing character
        if (typeSpecifier == 'c') {
            EMIT(va_arg(list, int));
        }

        // printing a null-terminated string
        if (typeSpecifier == 's') {
            char* val = va_arg(list, char*);
            size_t val_length = strlen(val);
            size_t pad_count = 0;
            if (val_length < width) {
                pad_count = width - val_length;
            }
            if (pad_count > 0 && !leftAlign) {
                for (size_t i = 0; i < pad_count; i++) {
                    EMIT(zeroPad ? '0' : ' ');
                }
            }
            if (!emit_str(out, &chars, val)) return chars;
            if (pad_count > 0 && leftAlign) {
                for (size_t i = 0; i < pad_count; i++) {
                    EMIT(zeroPad ? '0' : ' ');
                }
            }
        }

        // print nothing, but write number of characters successfully written so
        // far into an integer pointer parameter
        if (typeSpecifier == 'n') {
            switch (size) {
                case 'H':
                    *(va_arg(list, signed char*)) = chars;
                    break;
                case 'h':
                    *(va_arg(list, short int*)) = chars;
                    break;
                case 0:
                    *(va_arg(list, int*)) = chars;
                    break;
                case 'l':
                    *(va_arg(list, long*)) = chars;
                    break;
                case 'q':
                    *(va_arg(list, long long*)) = chars;
                    break;
                case 'j':
                    *(va_arg(list, intmax_t*)) = chars;
                    break;
                case 'z':
                    *(va_arg(list, size_t*)) = chars;
                    break;
                case 't':
                    *(va_arg(list, ptrdiff_t*)) = chars;
                    break;
                default:
                    break;
            }
        }

        // handle doubles
        bool eMode = false;
        bool eModeUppercase = false;
        int expo = 0;
        if (typeSpecifier == 'e') {
            eMode = true;
            typeSpecifier = 'f';
        }
        if (typeSpecifier == 'E') {
            eMode = true;
            eModeUppercase = true;
            typeSpecifier = 'F';
        }

        if (typeSpecifier == 'f' || typeSpecifier == 'F') {
            double floating = va_arg(list, double);
            while (eMode && (floating >= 10 || floating <= -10)) {
                floating /= 10;
                expo++;
            }
            while (eMode && floating < 1 && floating > -1) {
                floating *= 10;
                expo--;
            }

            // Compute integer and fractional parts with rounding before
            // emitting, so that a carry from the fractional part can propagate
            // to the integer.
            intmax_t intPart = (intmax_t)floating;
            double frac = floating - intPart;
            if (frac < 0) frac *= -1;

            for (int i = 0; i < precision; i++) {
                frac *= 10;
            }

            intmax_t decPlaces = (intmax_t)frac;
            uint8_t nextDigit = (uint8_t)(
                (frac * 10) - (decPlaces * 10) + 0.01
            );
            if (nextDigit >= 5) {
                decPlaces++;
            }

            // Propagate carry into integer part if fractional digits overflow.
            intmax_t divisor = 1;
            for (int i = 0; i < precision; i++) divisor *= 10;
            if (decPlaces >= divisor) {
                decPlaces -= divisor;
                intPart += (floating >= 0) ? 1 : -1;
            }

            int form = width - precision - expo -
                (precision || altMode ? 1 : 0);

            if (eMode) {
                form -= 4;
            }
            if (form < 0) {
                form = 0;
            }

            __int_to_str(
                intPart, intStrBuffer, 10, plusPrepend,
                spacePrepend, form, leftAlign, zeroPad
            );

            if (!emit_str(out, &chars, intStrBuffer)) return chars;

            if (precision) {
                EMIT('.');
                __int_to_str(
                    decPlaces, intStrBuffer, 10, false,
                    false, precision, false, true
                );
                intStrBuffer[precision] = 0;
                if (!emit_str(out, &chars, intStrBuffer)) return chars;
            } else if (altMode) {
                EMIT('.');
            }

            if (eMode) {
                EMIT(eModeUppercase ? 'E' : 'e');
                __int_to_str(
                    expo, intStrBuffer, 10, false, false, 0, false, false);
                if (!emit_str(out, &chars, intStrBuffer)) return chars;
            }
        }

        // Display a percentage character
        if (typeSpecifier == '%') {
            EMIT('%');
        }
    }

    return chars;
}
