#include <stdlib.h>
#include <ctype.h>

int atoi(const char* str)
{
	int result = 0;
	int sign = 1;
	
	/* Skip leading whitespace */
	while (isspace(*str)) {
		str++;
	}
	
	/* Handle optional sign */
	if (*str == '-') {
		sign = -1;
		str++;
	} else if (*str == '+') {
		str++;
	}
	
	/* Convert digits */
	while (isdigit(*str)) {
		result = result * 10 + (*str - '0');
		str++;
	}
	
	return sign * result;
}