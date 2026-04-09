#if !(defined(__is_libk))

#include <string.h>
#include "./_stdlib_environ.h"

char* getenv(const char* name)
{
    if (environ == NULL || name == NULL) {
        return NULL;
    }
    size_t name_len = strlen(name);
    for (char** entry = environ; *entry != NULL; entry++) {
        if (strncmp(*entry, name, name_len) == 0 && (*entry)[name_len] == '=') {
            return *entry + name_len + 1;
        }
    }
    return NULL;
}

#endif
