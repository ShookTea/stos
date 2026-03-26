#include <string.h>

char* strchr(const char* s, int c)
{
	char ch = (char)c;
	
	while (*s) {
		if (*s == ch) {
			return (char*)s;
		}
		s++;
	}
	
	/* Check if we're looking for the null terminator */
	if (ch == '\0') {
		return (char*)s;
	}
	
	return NULL;
}