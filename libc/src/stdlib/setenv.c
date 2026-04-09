#if !(defined(__is_libk))

#include <stdlib.h>
#include <string.h>
#include "./_stdlib_environ.h"

int setenv(const char* name, const char* value, int overwrite)
{
    if (name == NULL || name[0] == '\0' || strchr(name, '=') != NULL) {
        return -1;
    }

    int index = __stdlib_environ_find(name);
    if (index >= 0 && !overwrite) {
        return 0;
    }

    size_t name_len = strlen(name);
    size_t val_len = strlen(value);
    char* entry = malloc(name_len + val_len + 2); /* '=' + '\0' */
    if (entry == NULL) {
        return -1;
    }
    memcpy(entry, name, name_len);
    entry[name_len] = '=';
    memcpy(entry + name_len + 1, value, val_len + 1);

    if (index >= 0) {
        /* old string is not freed — may have been passed via putenv */
        environ[index] = entry;
        return 0;
    }

    return __stdlib_environ_append(entry);
}

#endif
