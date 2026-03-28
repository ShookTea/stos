#include <stdio.h>

#if defined(__is_libk)
    #include <kernel/terminal.h>
#else
    #include <unistd.h>
#endif

int putchar(int ic)
{
    char c = (char) ic;
    #if defined(__is_libk)
        terminal_write(&c, sizeof(c));
    #else
        write(1, &c, 1);
    #endif
    return ic;
}
