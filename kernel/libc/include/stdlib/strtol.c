#include <stdlib.h>
#include <ctype.h>

long strtol(const char* str, char** endptr, int base)
{
	long result = 0;
	int sign = 1;
	const char* start = str;
	
	/* Validate base */
	if (base < 0 || base == 1 || base > 36) {
		if (endptr) {
			*endptr = (char*)str;
		}
		return 0;
	}
	
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
	
	/* Handle base detection and prefixes */
	if (base == 0) {
		if (*str == '0') {
			if ((str[1] == 'x' || str[1] == 'X') && isxdigit(str[2])) {
				base = 16;
				str += 2;
			} else {
				base = 8;
			}
		} else {
			base = 10;
		}
	} else if (base == 16) {
		/* Allow optional "0x" or "0X" prefix for base 16 */
		if (*str == '0' && (str[1] == 'x' || str[1] == 'X') && isxdigit(str[2])) {
			str += 2;
		}
	}
	
	/* Convert digits */
	int found_digit = 0;
	while (*str) {
		int digit;
		
		if (isdigit(*str)) {
			digit = *str - '0';
		} else if (isalpha(*str)) {
			if (isupper(*str)) {
				digit = *str - 'A' + 10;
			} else {
				digit = *str - 'a' + 10;
			}
		} else {
			break;
		}
		
		/* Check if digit is valid for the base */
		if (digit >= base) {
			break;
		}
		
		found_digit = 1;
		result = result * base + digit;
		str++;
	}
	
	/* Set endptr */
	if (endptr) {
		if (found_digit) {
			*endptr = (char*)str;
		} else {
			*endptr = (char*)start;
		}
	}
	
	return sign * result;
}