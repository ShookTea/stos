#ifndef _STDLIB_SRC_STDIO_FORMAT_H
#define _STDLIB_SRC_STDIO_FORMAT_H

#include <stddef.h>
#include <stdarg.h>

typedef struct __stdio_format_out {
    /**
     * Function that emits a character and returns the number of outputted
     * characters. When 0 is returned, that means that the output is full
     * and printing should be stopped.
     */
    int (*emit_char)(struct __stdio_format_out*, char);
    /** Different output channels */
    union {
        /** File descriptor ID (unistd.h -> write()) */
        struct { int fd; } fd;
        /** String buffer */
        struct {
            /** Pointer to buffer */
            char* s;
            /** Max characters */
            size_t cap;
            /** Current number of printed characters */
            size_t len;
        } str;
    } output;
} __stdio_format_out_t;

/**
 * Runs formatted printing based on configured output.
 */
int __stdio_format_core(__stdio_format_out_t* out, const char* fmt, va_list op);

#endif
