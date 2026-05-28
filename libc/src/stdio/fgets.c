#if !(defined(__is_libk))

#include <stdio.h>

char* fgets(char* restrict s, int size, FILE* restrict stream)
{
    if (size <= 0 || s == NULL || stream == NULL) {
         return NULL;
    }

    int i = 0;
    while (i < size - 1) {
        int c = fgetc(stream);
        if (c == EOF) {
            if (i == 0) return NULL;
            break;
        }
        s[i++] = (char)c;
        if (c == '\n') break;
    }
    s[i] = '\0';
    return s;
}

#endif
