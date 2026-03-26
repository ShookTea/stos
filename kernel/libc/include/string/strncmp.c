#include <string.h>

int strncmp(const char* s1, const char* s2, size_t size)
{
	const unsigned char* a = (const unsigned char*) s1;
	const unsigned char* b = (const unsigned char*) s2;
	unsigned char c1 = '\0', c2 = '\0';

	while (size > 0) {
	    c1 = *a++;
		c2 = *b++;
		if (c1 == '\0' || c1 != c2) {
		    return c1 - c2;
		}
		size--;
	}
	return c1 - c2;
}
