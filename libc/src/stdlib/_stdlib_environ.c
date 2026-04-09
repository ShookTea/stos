#if !(defined(__is_libk))

#include "./_stdlib_environ.h"
#include <libds/libds.h>
#include <libds/strdict.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/**
 * Array of pointers to strings containing env variables, in name=value format.
 * The array is terminated with a null pointer.
 */
char** environ;

ds_strdict_t* __stdlib_environ_get_dict()
{
    if (environ == NULL) {
        return NULL;
    }
    return ds_strdict_create_environ((const char**)environ);
}

size_t __stdlib_environ_size()
{
    if (environ == NULL) {
        return 0;
    }
    size_t n = 0;
    while (environ[n] != NULL) n++;
    return n;
}

int __stdlib_environ_find(const char* name)
{
    if (environ == NULL || name == NULL) {
        return -1;
    }
    size_t name_len = strlen(name);
    for (size_t i = 0; environ[i] != NULL; i++) {
        if (strncmp(environ[i], name, name_len) == 0
            && environ[i][name_len] == '='
        ) {
            return (int)i;
        }
    }
    return -1;
}

int __stdlib_environ_append(char* entry)
{
    size_t size = __stdlib_environ_size();
    char** new_environ = realloc(environ, sizeof(char*) * (size + 2));
    if (new_environ == NULL) {
        return -1;
    }
    environ = new_environ;
    environ[size] = entry;
    environ[size + 1] = NULL;
    return 0;
}

void __stdlib_environ_remove_index(size_t index)
{
    size_t size = __stdlib_environ_size();
    for (size_t i = index; i < size - 1; i++) {
        environ[i] = environ[i + 1];
    }
    environ[size - 1] = NULL;
}

#endif
