#if !(defined(__is_libk))

#include <string.h>
#include "./_stdlib_environ.h"

int putenv(char* string)
{
    if (string == NULL || strchr(string, '=') == NULL) {
        return -1;
    }

    size_t name_len = strchr(string, '=') - string;
    if (name_len == 0) {
        return -1;
    }

    // find existing entry with the same name
    if (environ != NULL) {
        for (size_t i = 0; environ[i] != NULL; i++) {
            if (strncmp(environ[i], string, name_len) == 0
                && environ[i][name_len] == '='
            ) {
                // replace pointer directly — no copy, no free of old string
                environ[i] = string;
                return 0;
            }
        }
    }

    return __stdlib_environ_append(string);
}

#endif
