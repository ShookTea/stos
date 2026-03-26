#include <string.h>

int strcmp(const char* s1, const char* s2)
{
	const unsigned char* a = (const unsigned char*) s1;
	const unsigned char* b = (const unsigned char*) s2;

	while (*a && (*a == *b)) {
		a++;
		b++;
	}

	return *a - *b;
}