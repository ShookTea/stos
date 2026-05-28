#include <string.h>

char* strtok(char* str, const char* delim)
{
	static char* saved_str = NULL;
	return strtok_r(str, delim, &saved_str);
}
