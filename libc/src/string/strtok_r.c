#include <string.h>

char* strtok_r(
    char* restrict str,
    const char* restrict delim,
    char** restrict saveptr
) {
	char* token_start;

	/* If str is NULL, continue from where we left off */
	if (str == NULL) {
		str = *saveptr;
	}

	/* If we have no string to work with, return NULL */
	if (str == NULL) {
		return NULL;
	}

	/* Skip leading delimiters */
	while (*str && strchr(delim, *str)) {
		str++;
	}

	/* If we've reached the end, no more tokens */
	if (*str == '\0') {
		*saveptr = NULL;
		return NULL;
	}

	/* This is the start of the token */
	token_start = str;

	/* Find the end of the token */
	while (*str && !strchr(delim, *str)) {
		str++;
	}

	/* If we found a delimiter, replace it with null terminator */
	if (*str) {
		*str = '\0';
		*saveptr = str + 1;
	} else {
		/* We've reached the end of the string */
		*saveptr = NULL;
	}

	return token_start;
}
