#ifndef _LIBDS_STRDICT
#define _LIBDS_STRDICT

#include <stddef.h>
#include <stdbool.h>
#include <libds/libds.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Type definition for string dictionary, for external use.
 */
typedef struct ds_strdict_s ds_strdict_t;

/**
 * Creates an empty string dictionary that grows automatically as necessary.
 * Caller is responsible for running ds_strdict_destroy to deallocate the memory
 * later.
 */
ds_strdict_t* ds_strdict_create();

/**
 * Creates a strict dictionary and attempts to parse NULL-terminated environ as
 * key=value pairs. Will skip invalid entries.
 *
 * Caller is responsible for running ds_strdict_destroy to deallocate the memory
 * later.
 */
ds_strdict_t* ds_strdict_create_environ(const char** environ);

/**
 * Destroys the string dictionary and deallocates the memory.
 */
void ds_strdict_destroy(ds_strdict_t*);

/**
 * Sets value under given key in the dictionary. Returns DS_SUCCESS on success.
 * If dictionary already has a key and "overwrite" is set to true, the value
 * will be overwriten. Otherwise, function will return DS_ERR_DUPLICATE.
 */
ds_result_t ds_strdict_set(
    ds_strdict_t* dict,
    const char* key,
    const char* value,
    const bool overwrite
);

/**
 * Adds value under given key in the dictionary, silently ignoring case when
 * key already exists. It's equivalent of calling ds_strdict_set with overwrite
 * set to "false" and ignoring DS_ERR_DUPLICATE, returning DS_SUCCESS instead.
 */
inline ds_result_t ds_strdict_add(
    ds_strdict_t* dict,
    const char* key,
    const char* value
) {
    ds_result_t res = ds_strdict_set(dict, key, value, false);
    return res == DS_ERR_DUPLICATE ? DS_SUCCESS : res;
}

/**
 * Tries to find value in the dictionary under given key and store it under
 * address in "val". Returns DS_SUCCESS on successful read, or DS_ERR_NOT_FOUND
 * if given key doesn't exist in the dictionary.
 */
ds_result_t ds_strdict_get(
    const ds_strdict_t* dict,
    const char* key,
    const char** val
);

/**
 * Returns "true" if "dict" is a valid dictionary and contains given key,
 * "false" otherwise.
 */
bool ds_strdict_has(const ds_strdict_t* dict, const char* key);

/**
 * Removes all items from the dictionary.
 */
ds_result_t ds_strdict_clear(ds_strdict_t* dict);

/**
 * Removes item with given key from the dictionary. Returns DS_SUCCESS on
 * successful removal, or DS_ERR_NOT_FOUND if given key doesn't exist in the
 * dictionary.
 */
ds_result_t ds_strdict_remove(ds_strdict_t* dict, const char* key);

/**
 * Return the total number of items in the dictionary.
 */
size_t ds_strdict_size(const ds_strdict_t* dict);

/**
 * Returns "true" if given dictionary is empty, "false" otherwise.
 */
inline bool ds_strdict_is_empty(const ds_strdict_t* dict)
{
    return ds_strdict_size(dict) == 0;
}

/**
 * Dumps the content of the dictionary into a NULL-terminated environ-like
 * structure, in key=value format.
 *
 * It's a responsibility of a caller to free each string and then the array
 * itself once it's no longer needed.
 */
const char** ds_strdict_dump_environ(const ds_strdict_t* dict);

#ifdef __cplusplus
}
#endif

#endif
