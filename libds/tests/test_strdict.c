#include "runner.h"
#include <libds/strdict.h>
#include <libds/libds.h>
#include <stdlib.h>
#include <string.h>

/**
 * helpers
 */
static int alloc_balance = 0;

static void* track_malloc(size_t size)
{
    alloc_balance++;
    return malloc(size);
}
static void* track_realloc(void* ptr, size_t size)
{
    if (ptr == NULL) alloc_balance++;
    return realloc(ptr, size);
}
static void track_free(void* ptr) {
    if (ptr) alloc_balance--;
    free(ptr);
}

// Individual tests

static void test_create_destroy(void)
{
    ds_strdict_t* dict = ds_strdict_create();
    ASSERT(dict != NULL);
    ASSERT(ds_strdict_size(dict) == 0);
    ASSERT(ds_strdict_is_empty(dict));
    ds_strdict_destroy(dict);
}

static void test_set_and_get(void)
{
    ds_strdict_t* dict = ds_strdict_create();
    ASSERT(ds_strdict_set(dict, "key", "value", false) == DS_SUCCESS);
    const char* val = NULL;
    ASSERT(ds_strdict_get(dict, "key", &val) == DS_SUCCESS);
    ASSERT(strcmp(val, "value") == 0);
    ds_strdict_destroy(dict);
}

static void test_has(void)
{
    ds_strdict_t* dict = ds_strdict_create();
    ds_strdict_set(dict, "foo", "bar", false);
    ASSERT(ds_strdict_has(dict, "foo"));
    ASSERT(!ds_strdict_has(dict, "baz"));
    ds_strdict_destroy(dict);
}

static void test_get_not_found(void)
{
    ds_strdict_t* dict = ds_strdict_create();
    const char* val = NULL;
    ASSERT(ds_strdict_get(dict, "missing", &val) == DS_ERR_NOT_FOUND);
    ds_strdict_destroy(dict);
}

static void test_duplicate_no_overwrite(void)
{
    ds_strdict_t* dict = ds_strdict_create();
    ds_strdict_set(dict, "k", "v1", false);
    ASSERT(ds_strdict_set(dict, "k", "v2", false) == DS_ERR_DUPLICATE);
    /* original value must be unchanged */
    const char* val = NULL;
    ds_strdict_get(dict, "k", &val);
    ASSERT(strcmp(val, "v1") == 0);
    ds_strdict_destroy(dict);
}

static void test_overwrite(void)
{
    ds_strdict_t* dict = ds_strdict_create();
    ds_strdict_set(dict, "k", "old", false);
    ASSERT(ds_strdict_set(dict, "k", "new", true) == DS_SUCCESS);
    const char* val = NULL;
    ds_strdict_get(dict, "k", &val);
    ASSERT(strcmp(val, "new") == 0);
    ASSERT(ds_strdict_size(dict) == 1);
    ds_strdict_destroy(dict);
}

static void test_add_ignores_duplicate(void)
{
    ds_strdict_t* dict = ds_strdict_create();
    ds_strdict_set(dict, "k", "original", false);
    ASSERT(ds_strdict_add(dict, "k", "ignored") == DS_SUCCESS);
    const char* val = NULL;
    ds_strdict_get(dict, "k", &val);
    ASSERT(strcmp(val, "original") == 0);
    ds_strdict_destroy(dict);
}

static void test_remove(void)
{
    ds_strdict_t* dict = ds_strdict_create();
    ds_strdict_set(dict, "a", "1", false);
    ds_strdict_set(dict, "b", "2", false);
    ASSERT(ds_strdict_remove(dict, "a") == DS_SUCCESS);
    ASSERT(ds_strdict_size(dict) == 1);
    ASSERT(!ds_strdict_has(dict, "a"));
    ASSERT(ds_strdict_has(dict, "b"));
    ds_strdict_destroy(dict);
}

static void test_remove_not_found(void)
{
    ds_strdict_t* dict = ds_strdict_create();
    ASSERT(ds_strdict_remove(dict, "ghost") == DS_ERR_NOT_FOUND);
    ds_strdict_destroy(dict);
}

static void test_remove_preserves_others(void)
{
    ds_strdict_t* dict = ds_strdict_create();
    ds_strdict_set(dict, "a", "1", false);
    ds_strdict_set(dict, "b", "2", false);
    ds_strdict_set(dict, "c", "3", false);
    ds_strdict_remove(dict, "b");
    const char* val = NULL;
    ASSERT(ds_strdict_get(dict, "a", &val) == DS_SUCCESS);
    ASSERT(strcmp(val, "1") == 0);
    ASSERT(ds_strdict_get(dict, "c", &val) == DS_SUCCESS);
    ASSERT(strcmp(val, "3") == 0);
    ASSERT(ds_strdict_size(dict) == 2);
    ds_strdict_destroy(dict);
}

static void test_clear(void)
{
    ds_strdict_t* dict = ds_strdict_create();
    ds_strdict_set(dict, "x", "1", false);
    ds_strdict_set(dict, "y", "2", false);
    ASSERT(ds_strdict_clear(dict) == DS_SUCCESS);
    ASSERT(ds_strdict_size(dict) == 0);
    ASSERT(ds_strdict_is_empty(dict));
    /* must accept new insertions after clear */
    ASSERT(ds_strdict_set(dict, "x", "new", false) == DS_SUCCESS);
    ds_strdict_destroy(dict);
}

static void test_size(void)
{
    ds_strdict_t* dict = ds_strdict_create();
    ASSERT(ds_strdict_size(dict) == 0);
    ds_strdict_set(dict, "a", "1", false);
    ASSERT(ds_strdict_size(dict) == 1);
    ds_strdict_set(dict, "b", "2", false);
    ASSERT(ds_strdict_size(dict) == 2);
    ds_strdict_remove(dict, "a");
    ASSERT(ds_strdict_size(dict) == 1);
    ds_strdict_destroy(dict);
}

static void test_grow_beyond_initial_capacity(void)
{
    ds_strdict_t* dict = ds_strdict_create();
    /* insert more than DICT_GROW_COUNT (10) items to force a grow */
    char key[8], val[8];
    for (int i = 0; i < 15; i++) {
        snprintf(key, sizeof(key), "k%d", i);
        snprintf(val, sizeof(val), "v%d", i);
        ASSERT(ds_strdict_set(dict, key, val, false) == DS_SUCCESS);
    }
    ASSERT(ds_strdict_size(dict) == 15);
    /* spot-check a few values survive the grow */
    const char* out = NULL;
    ASSERT(ds_strdict_get(dict, "k0",  &out) == DS_SUCCESS); ASSERT(strcmp(out, "v0")  == 0);
    ASSERT(ds_strdict_get(dict, "k14", &out) == DS_SUCCESS); ASSERT(strcmp(out, "v14") == 0);
    ds_strdict_destroy(dict);
}

static void test_create_environ(void)
{
    const char* env[] = { "FOO=bar", "BAZ=qux", NULL };
    ds_strdict_t* dict = ds_strdict_create_environ(env);
    ASSERT(dict != NULL);
    ASSERT(ds_strdict_size(dict) == 2);
    const char* val = NULL;
    ASSERT(ds_strdict_get(dict, "FOO", &val) == DS_SUCCESS);
    ASSERT(strcmp(val, "bar") == 0);
    ASSERT(ds_strdict_get(dict, "BAZ", &val) == DS_SUCCESS);
    ASSERT(strcmp(val, "qux") == 0);
    ds_strdict_destroy(dict);
}

static void test_create_environ_skips_invalid(void)
{
    const char* env[] = { "VALID=yes", "NOEQUALSSIGN", "ALSO=ok", NULL };
    ds_strdict_t* dict = ds_strdict_create_environ(env);
    ASSERT(ds_strdict_size(dict) == 2);
    ASSERT(ds_strdict_has(dict, "VALID"));
    ASSERT(ds_strdict_has(dict, "ALSO"));
    ASSERT(!ds_strdict_has(dict, "NOEQUALSSIGN"));
    ds_strdict_destroy(dict);
}

static void test_create_environ_null(void)
{
    ds_strdict_t* dict = ds_strdict_create_environ(NULL);
    ASSERT(dict != NULL);
    ASSERT(ds_strdict_is_empty(dict));
    ds_strdict_destroy(dict);
}

static void test_create_environ_empty_value(void)
{
    const char* env[] = { "EMPTY=", NULL };
    ds_strdict_t* dict = ds_strdict_create_environ(env);
    ASSERT(ds_strdict_size(dict) == 1);
    const char* val = NULL;
    ASSERT(ds_strdict_get(dict, "EMPTY", &val) == DS_SUCCESS);
    ASSERT(strcmp(val, "") == 0);
    ds_strdict_destroy(dict);
}

static void test_dump_environ(void)
{
    ds_strdict_t* dict = ds_strdict_create();
    ds_strdict_set(dict, "A", "1", false);
    ds_strdict_set(dict, "B", "2", false);
    const char** env = ds_strdict_dump_environ(dict);
    ASSERT(env != NULL);
    /* must be NULL-terminated */
    ASSERT(env[2] == NULL);
    /* both entries must appear as KEY=VALUE */
    bool found_a = false, found_b = false;
    for (int i = 0; env[i] != NULL; i++) {
        if (strcmp(env[i], "A=1") == 0) found_a = true;
        if (strcmp(env[i], "B=2") == 0) found_b = true;
        free((void*)env[i]);
    }
    free(env);
    ASSERT(found_a);
    ASSERT(found_b);
    ds_strdict_destroy(dict);
}

static void test_dump_environ_roundtrip(void)
{
    ds_strdict_t* src = ds_strdict_create();
    ds_strdict_set(src, "X", "hello", false);
    ds_strdict_set(src, "Y", "world", false);
    const char** env = ds_strdict_dump_environ(src);

    ds_strdict_t* dst = ds_strdict_create_environ(env);
    ASSERT(ds_strdict_size(dst) == 2);
    const char* val = NULL;
    ASSERT(ds_strdict_get(dst, "X", &val) == DS_SUCCESS);
    ASSERT(strcmp(val, "hello") == 0);
    ASSERT(ds_strdict_get(dst, "Y", &val) == DS_SUCCESS);
    ASSERT(strcmp(val, "world") == 0);

    for (int i = 0; env[i] != NULL; i++) free((void*)env[i]);
    free(env);
    ds_strdict_destroy(src);
    ds_strdict_destroy(dst);
}

static void test_null_handling(void)
{
    const char* val = NULL;
    ASSERT(ds_strdict_set(NULL, "k", "v", false) == DS_ERR_INVALID);
    ASSERT(ds_strdict_get(NULL, "k", &val)        == DS_ERR_INVALID);
    ASSERT(ds_strdict_remove(NULL, "k")           == DS_ERR_INVALID);
    ASSERT(ds_strdict_clear(NULL)                 == DS_ERR_INVALID);
    ASSERT(ds_strdict_size(NULL)                  == 0);
    ASSERT(!ds_strdict_has(NULL, "k"));
    ASSERT(ds_strdict_dump_environ(NULL)          == NULL);
    ds_strdict_destroy(NULL); /* must not crash */
}

static void test_no_leaks(void)
{
    libds_set_allocators(track_malloc, track_realloc, track_free);
    int before = alloc_balance;

    ds_strdict_t* dict = ds_strdict_create();
    ds_strdict_set(dict, "a", "1", false);
    ds_strdict_set(dict, "b", "2", false);
    ds_strdict_set(dict, "a", "updated", true);
    ds_strdict_remove(dict, "b");
    ds_strdict_destroy(dict);

    ASSERT(alloc_balance == before);
    libds_set_allocators(malloc, realloc, free);
}

static void test_no_leaks_clear(void)
{
    libds_set_allocators(track_malloc, track_realloc, track_free);
    int before = alloc_balance;

    ds_strdict_t* dict = ds_strdict_create();
    ds_strdict_set(dict, "x", "1", false);
    ds_strdict_set(dict, "y", "2", false);
    ds_strdict_clear(dict);
    ds_strdict_destroy(dict);

    ASSERT(alloc_balance == before);
    libds_set_allocators(malloc, realloc, free);
}

// Suite entry point

void test_strdict(void)
{
    RUN_TEST(test_create_destroy);
    RUN_TEST(test_set_and_get);
    RUN_TEST(test_has);
    RUN_TEST(test_get_not_found);
    RUN_TEST(test_duplicate_no_overwrite);
    RUN_TEST(test_overwrite);
    RUN_TEST(test_add_ignores_duplicate);
    RUN_TEST(test_remove);
    RUN_TEST(test_remove_not_found);
    RUN_TEST(test_remove_preserves_others);
    RUN_TEST(test_clear);
    RUN_TEST(test_size);
    RUN_TEST(test_grow_beyond_initial_capacity);
    RUN_TEST(test_create_environ);
    RUN_TEST(test_create_environ_skips_invalid);
    RUN_TEST(test_create_environ_null);
    RUN_TEST(test_create_environ_empty_value);
    RUN_TEST(test_dump_environ);
    RUN_TEST(test_dump_environ_roundtrip);
    RUN_TEST(test_null_handling);
    RUN_TEST(test_no_leaks);
    RUN_TEST(test_no_leaks_clear);
}
