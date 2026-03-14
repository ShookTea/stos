#include <string.h>

size_t strspn(const char* s, const char* accept)
{
	size_t count = 0;
	const char* a;
	
	while (*s) {
		int found = 0;
		for (a = accept; *a; a++) {
			if (*s == *a) {
				found = 1;
				break;
			}
		}
		if (!found) {
			break;
		}
		count++;
		s++;
	}
	
	return count;
}