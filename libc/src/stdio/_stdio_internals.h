#ifndef _STDLIB_SRC_STDIO_INTERNALS_H
#define _STDLIB_SRC_STDIO_INTERNALS_H

void __stdio_init(void);
int __stdio_internals_fcntl_flags_from_mode(const char* restrict mode);

#endif
