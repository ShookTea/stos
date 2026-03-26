#include <string.h>

char* strcat(char* dest, const char* src)
{
    size_t dest_length, source_length;

    for (dest_length = 0; dest[dest_length] != '\0'; dest_length++) {
        // do nothing - we're counting the number of characters
    }
    for (source_length = 0; src[source_length] != '\0'; source_length++) {
        dest[dest_length + source_length] = src[source_length];
    }
    dest[dest_length+source_length] = '\0';

    return dest;
}
