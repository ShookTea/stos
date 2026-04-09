#if !(defined(__is_libk))

#include <string.h>
#include "./_stdlib_environ.h"

int unsetenv(const char* name)
{
    if (name == NULL || name[0] == '\0' || strchr(name, '=') != NULL) {
        return -1;
    }

    int index = __stdlib_environ_find(name);
    if (index < 0) {
        return 0; /* not found is not an error */
    }

    __stdlib_environ_remove_index((size_t)index);
    return 0;
}

#endif
