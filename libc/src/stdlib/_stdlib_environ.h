#if !(defined(__is_libk))

#ifndef _STDLIB_SRC_STDLIB_ENVIRON_H
#define _STDLIB_SRC_STDLIB_ENVIRON_H

#include <stddef.h>

extern char** environ;

/**
 * Returns the number of entries in environ (not counting NULL terminator).
 */
size_t __stdlib_environ_size();

/**
 * Returns the index of the environ entry matching name, or -1 if not found.
 */
int __stdlib_environ_find(const char* name);

/**
 * Appends entry to environ, growing the array. Returns 0 on success, -1 on
 * allocation failure.
 */
int __stdlib_environ_append(char* entry);

/**
 * Removes the entry at given index, shifting remaining entries left.
 */
void __stdlib_environ_remove_index(size_t index);

#endif
#endif
