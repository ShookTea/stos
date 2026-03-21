#include <stdio.h>

#if defined(__is_libk)
#include <kernel/terminal.h>
#endif

int putchar(int ic)
{
    #if defined(__is_libk)
    char c = (char) ic;
    terminal_write(&c, sizeof(c));
    return ic;
    #else
    // TODO: implement stdio for userspace and write system call
    return EOF;
    #endif
}
