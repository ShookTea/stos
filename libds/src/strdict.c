#include <libds/strdict.h>
#include <libds/libds.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

// By how many key-value pairs do we grow the dictionary if needed
#define DICT_GROW_COUNT 10

struct ds_strdict_s {
    size_t capacity;
    /** Count of key-value pairs (not "items" below) */
    size_t count;
    /**
     * Each item is stored as two entries here (one for key and one for value),
     * so for item N it's key is at items[N/2] and value is at items[N/2 + 1].
     */
    char** items;
};

/**
 * Increase the capacity of the dict by 10 key-value pairs (20 "items")
 */
static void dict_grow(ds_strdict_t* dict)
{
    size_t old_capacity = dict->capacity;
    dict->capacity += DICT_GROW_COUNT;
    dict->items = libds_realloc(
        dict->items,
        sizeof(char*) * dict->capacity * 2
    );
    // Clear empty data
    memset(
        dict->items + (old_capacity * 2),
        0,
        sizeof(char*) * DICT_GROW_COUNT * 2
    );
}

/**
 * Tries to find index of given key in the dictionary. It returns index of the
 * key-value pair - so if returned value is N, then key lives at index N*2 and
 * value lives at N*2+1
 */
static int dict_find_key(const ds_strdict_t* dict, const char* key)
{
    for (size_t i = 0; i < dict->count; i++) {
        if (strcmp(key, dict->items[i*2]) == 0) {
            return i;
        }
    }
    return -1;
}

ds_strdict_t* ds_strdict_create()
{
    ds_strdict_t* dict = libds_malloc(sizeof(ds_strdict_t));
    dict->capacity = 0;
    dict->count = 0;
    dict->items = NULL;
    return dict;
}

ds_strdict_t* ds_strdict_create_environ(const char** environ)
{
    if (environ == NULL) {
        return ds_strdict_create();
    }

    size_t entries_count = 0;
    const char** current = environ;
    while (*current != NULL) {
        entries_count++;
        current++;
    }

    ds_strdict_t* dict = ds_strdict_create();
    dict->capacity = entries_count;
    dict->items = NULL;
    if (entries_count > 0) {
        dict->items = libds_malloc(sizeof(char*) * entries_count * 2);
    }
    dict->count = 0;

    for (size_t i = 0; i < entries_count; i++) {
        const char* entry = environ[i];
        const char* eq = strchr(entry, '=');
        if (eq == NULL) continue; // invalid entry
        size_t key_len = eq - entry;
        size_t val_len = strlen(eq + 1);
        char* key = libds_malloc(key_len + 1);
        char* val = libds_malloc(val_len + 1);
        memcpy(key, entry, key_len);
        key[key_len] = '\0';
        memcpy(val, eq + 1, val_len + 1);
        dict->items[dict->count * 2] = key;
        dict->items[dict->count * 2 + 1] = val;
        dict->count++;
    }

    return dict;
}

void ds_strdict_destroy(ds_strdict_t* dict)
{
    if (dict == NULL) {
        return;
    }
    ds_strdict_clear(dict);
    libds_free(dict);
}

ds_result_t ds_strdict_set(
    ds_strdict_t* dict,
    const char* key,
    const char* value,
    const bool overwrite
) {
    if (dict == NULL) {
        return DS_ERR_INVALID;
    }

    int existing_index = dict_find_key(dict, key);
    if (existing_index >= 0 && !overwrite) {
        return DS_ERR_DUPLICATE;
    }

    if (existing_index >= 0) {
        // Replace existing value
        size_t value_idx = existing_index * 2 + 1;
        libds_free(dict->items[value_idx]);
        dict->items[value_idx] = libds_malloc(strlen(value) + 1);
        strcpy(dict->items[value_idx], value);
        return DS_SUCCESS;
    }

    if (dict->capacity == dict->count) {
        dict_grow(dict);
    }
    // Current count points to the first empty entry
    size_t key_idx = dict->count * 2;
    dict->items[key_idx] = libds_malloc(strlen(key) + 1);
    dict->items[key_idx + 1] = libds_malloc(strlen(value) + 1);
    strcpy(dict->items[key_idx], key);
    strcpy(dict->items[key_idx + 1], value);
    dict->count++;

    return DS_SUCCESS;
}

ds_result_t ds_strdict_get(
    const ds_strdict_t* dict,
    const char* key,
    const char** val
) {
    if (dict == NULL) {
        return DS_ERR_INVALID;
    }

    int index = dict_find_key(dict, key);
    if (index < 0) {
        return DS_ERR_NOT_FOUND;
    }

    size_t val_idx = index * 2 + 1;
    *val = dict->items[val_idx];
    return DS_SUCCESS;
}

bool ds_strdict_has(const ds_strdict_t* dict, const char* key)
{
    if (dict == NULL) {
        return false;
    }

    return dict_find_key(dict, key) >= 0;
}

ds_result_t ds_strdict_clear(ds_strdict_t* dict)
{
    if (dict == NULL) {
        return DS_ERR_INVALID;
    }

    if (dict->items != NULL) {
        for (size_t i = 0; i < dict->count * 2; i++) {
            libds_free(dict->items[i]);
        }
        libds_free(dict->items);
    }
    dict->items = NULL;
    dict->capacity = 0;
    dict->count = 0;
    return DS_SUCCESS;
}

ds_result_t ds_strdict_remove(ds_strdict_t* dict, const char* key)
{
    if (dict == NULL) {
        return DS_ERR_INVALID;
    }

    int index = dict_find_key(dict, key);
    if (index < 0) {
        return DS_ERR_NOT_FOUND;
    }

    size_t key_idx = index * 2;
    libds_free(dict->items[key_idx]);
    libds_free(dict->items[key_idx + 1]);

    size_t items_to_move = (dict->count - (size_t)index - 1) * 2;
    if (items_to_move > 0) {
        memmove(
            dict->items + key_idx,
            dict->items + key_idx + 2,
            sizeof(char*) * items_to_move
        );
    }

    dict->count--;
    dict->items[dict->count * 2] = NULL;
    dict->items[dict->count * 2 + 1] = NULL;
    return DS_SUCCESS;
}

size_t ds_strdict_size(const ds_strdict_t* dict)
{
    if (dict == NULL) {
        return 0;
    }

    return dict->count;
}

const char** ds_strdict_dump_environ(const ds_strdict_t* dict)
{
    if (dict == NULL) {
        return NULL;
    }

    const char** result = libds_malloc(sizeof(char*) * (dict->count + 1));
    for (size_t i = 0; i < dict->count; i++) {
        const char* key = dict->items[i * 2];
        const char* val = dict->items[i * 2 + 1];
        size_t key_len = strlen(key);
        size_t len = key_len + strlen(val) + 2; // '=' and '\0'
        char* entry = libds_malloc(len);
        strcpy(entry, key);
        entry[key_len] = '=';
        strcpy(entry + key_len + 1, val);
        result[i] = entry;
    }

    result[dict->count] = NULL;
    return result;
}
