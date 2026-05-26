#include <string.h>

char *strstr(const char *haystack, const char *needle)
{
    return memmem(haystack, strlen(haystack), needle, strlen(needle));
}
