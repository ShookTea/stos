#if !(defined(__is_libk))

#include <stdio.h>

int getchar(void)
{
    return fgetc(stdin);
}

#endif
