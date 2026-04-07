#include "runner.h"
#include <libds/ringbuf.h>
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
static void track_free(void* ptr) {
    if (ptr) alloc_balance--;
    free(ptr);
}

// Individual tests

static void test_push_pop_basic(void)
{
    ds_ringbuf_t* rb = ds_ringbuf_create(4, sizeof(int), true);
    int in = 42, out = 0;
    ASSERT(ds_ringbuf_push(rb, &in) == DS_SUCCESS);
    ASSERT(ds_ringbuf_pop(rb, &out) == DS_SUCCESS);
    ASSERT(out == 42);
    ds_ringbuf_destroy(rb);
}

static void test_push_peek(void)
{
    ds_ringbuf_t* rb = ds_ringbuf_create(4, sizeof(int), true);
    int in = 7, out = 0;
    ds_ringbuf_push(rb, &in);
    ASSERT(ds_ringbuf_peek(rb, &out) == DS_SUCCESS);
    ASSERT(out == 7);
    out = 0;
    /* peek must not consume the item */
    ASSERT(ds_ringbuf_peek(rb, &out) == DS_SUCCESS);
    ASSERT(out == 7);
    ASSERT(ds_ringbuf_size(rb) == 1);
    ds_ringbuf_destroy(rb);
}

static void test_fifo_order(void)
{
    ds_ringbuf_t* rb = ds_ringbuf_create(4, sizeof(int), true);
    for (int i = 1; i <= 4; i++)
        ds_ringbuf_push(rb, &i);
    for (int i = 1; i <= 4; i++) {
        int out = 0;
        ds_ringbuf_pop(rb, &out);
        ASSERT(out == i);
    }
    ds_ringbuf_destroy(rb);
}

static void test_pop_empty(void)
{
    ds_ringbuf_t* rb = ds_ringbuf_create(4, sizeof(int), true);
    int out;
    ASSERT(ds_ringbuf_pop(rb, &out) == DS_ERR_EMPTY);
    ds_ringbuf_destroy(rb);
}

static void test_peek_empty(void)
{
    ds_ringbuf_t* rb = ds_ringbuf_create(4, sizeof(int), true);
    int out;
    ASSERT(ds_ringbuf_peek(rb, &out) == DS_ERR_EMPTY);
    ds_ringbuf_destroy(rb);
}

static void test_fail_on_full(void)
{
    ds_ringbuf_t* rb = ds_ringbuf_create(3, sizeof(int), true);
    for (int i = 1; i <= 3; i++)
        ds_ringbuf_push(rb, &i);
    int extra = 99;
    ASSERT(ds_ringbuf_push(rb, &extra) == DS_ERR_FULL);
    /* existing data must be untouched */
    int out = 0;
    ds_ringbuf_pop(rb, &out);
    ASSERT(out == 1);
    ds_ringbuf_destroy(rb);
}

static void test_overwrite_oldest(void)
{
    ds_ringbuf_t* rb = ds_ringbuf_create(3, sizeof(int), false);
    for (int i = 1; i <= 3; i++)
        ds_ringbuf_push(rb, &i);
    /* push past capacity — oldest (1) must be dropped */
    int v = 4;
    ASSERT(ds_ringbuf_push(rb, &v) == DS_SUCCESS);
    ASSERT(ds_ringbuf_size(rb) == 3);
    int out = 0;
    ds_ringbuf_pop(rb, &out); ASSERT(out == 2);
    ds_ringbuf_pop(rb, &out); ASSERT(out == 3);
    ds_ringbuf_pop(rb, &out); ASSERT(out == 4);
    ds_ringbuf_destroy(rb);
}

static void test_wraparound(void)
{
    ds_ringbuf_t* rb = ds_ringbuf_create(4, sizeof(int), true);
    /* fill completely */
    for (int i = 1; i <= 4; i++)
        ds_ringbuf_push(rb, &i);
    /* drain two */
    int out;
    ds_ringbuf_pop(rb, &out); ASSERT(out == 1);
    ds_ringbuf_pop(rb, &out); ASSERT(out == 2);
    /* refill two — write pointer wraps around storage end */
    int v5 = 5, v6 = 6;
    ds_ringbuf_push(rb, &v5);
    ds_ringbuf_push(rb, &v6);
    /* drain all four remaining in order */
    ds_ringbuf_pop(rb, &out); ASSERT(out == 3);
    ds_ringbuf_pop(rb, &out); ASSERT(out == 4);
    ds_ringbuf_pop(rb, &out); ASSERT(out == 5);
    ds_ringbuf_pop(rb, &out); ASSERT(out == 6);
    ASSERT(ds_ringbuf_is_empty(rb));
    ds_ringbuf_destroy(rb);
}

static void test_clear(void)
{
    ds_ringbuf_t* rb = ds_ringbuf_create(4, sizeof(int), true);
    for (int i = 0; i < 4; i++)
        ds_ringbuf_push(rb, &i);
    ASSERT(ds_ringbuf_clear(rb) == DS_SUCCESS);
    ASSERT(ds_ringbuf_size(rb) == 0);
    ASSERT(ds_ringbuf_is_empty(rb));
    /* must accept new data after clear */
    int v = 1;
    ASSERT(ds_ringbuf_push(rb, &v) == DS_SUCCESS);
    ds_ringbuf_destroy(rb);
}

static void test_size_and_capacity(void)
{
    ds_ringbuf_t* rb = ds_ringbuf_create(5, sizeof(int), true);
    ASSERT(ds_ringbuf_capacity(rb) == 5);
    ASSERT(ds_ringbuf_size(rb) == 0);
    ASSERT(ds_ringbuf_free_space(rb) == 5);
    ASSERT(ds_ringbuf_is_empty(rb));
    ASSERT(!ds_ringbuf_is_full(rb));

    for (int i = 0; i < 5; i++)
        ds_ringbuf_push(rb, &i);
    ASSERT(ds_ringbuf_size(rb) == 5);
    ASSERT(ds_ringbuf_free_space(rb) == 0);
    ASSERT(ds_ringbuf_is_full(rb));
    ASSERT(!ds_ringbuf_is_empty(rb));
    ds_ringbuf_destroy(rb);
}

static void test_struct_item_size(void)
{
    typedef struct { int x; int y; char label[8]; } point_t;
    ds_ringbuf_t* rb = ds_ringbuf_create(2, sizeof(point_t), true);

    point_t a = {1, 2, "hello"};
    point_t b = {3, 4, "world"};
    ds_ringbuf_push(rb, &a);
    ds_ringbuf_push(rb, &b);

    point_t out;
    ds_ringbuf_pop(rb, &out);
    ASSERT(out.x == 1 && out.y == 2 && strcmp(out.label, "hello") == 0);
    ds_ringbuf_pop(rb, &out);
    ASSERT(out.x == 3 && out.y == 4 && strcmp(out.label, "world") == 0);
    ds_ringbuf_destroy(rb);
}

static void test_null_handling(void)
{
    int v = 0;
    ASSERT(ds_ringbuf_push(NULL, &v) == DS_ERR_INVALID);
    ASSERT(ds_ringbuf_pop(NULL, &v)  == DS_ERR_INVALID);
    ASSERT(ds_ringbuf_peek(NULL, &v) == DS_ERR_INVALID);
    ASSERT(ds_ringbuf_clear(NULL)    == DS_ERR_INVALID);
    ASSERT(ds_ringbuf_size(NULL)     == 0);
    ASSERT(ds_ringbuf_capacity(NULL) == 0);
    ds_ringbuf_destroy(NULL); /* must not crash */
}

static void test_no_leaks(void)
{
    libds_set_allocators(track_malloc, realloc, track_free);
    int before = alloc_balance;

    ds_ringbuf_t* rb = ds_ringbuf_create(4, sizeof(int), true);
    int v = 1;
    ds_ringbuf_push(rb, &v);
    ds_ringbuf_destroy(rb);

    ASSERT(alloc_balance == before);
    libds_set_allocators(malloc, realloc, free);
}

// Suite entry point

void test_ringbuf(void)
{
    RUN_TEST(test_push_pop_basic);
    RUN_TEST(test_push_peek);
    RUN_TEST(test_fifo_order);
    RUN_TEST(test_pop_empty);
    RUN_TEST(test_peek_empty);
    RUN_TEST(test_fail_on_full);
    RUN_TEST(test_overwrite_oldest);
    RUN_TEST(test_wraparound);
    RUN_TEST(test_clear);
    RUN_TEST(test_size_and_capacity);
    RUN_TEST(test_struct_item_size);
    RUN_TEST(test_null_handling);
    RUN_TEST(test_no_leaks);
}
