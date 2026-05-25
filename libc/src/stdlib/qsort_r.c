#include <stdbool.h>
#include <stdlib.h>

static void swap(char* a, char* b, size_t size)
{
    char tmp;
    while (size--) {
        tmp = *a;
        *a++ = *b;
        *b++ = tmp;
    }
}

static void sift_down(
    char* base,
    size_t root,
    size_t end,
    size_t size,
    int (*cmp)(const void* a, const void* b, void* arg),
    void* arg
) {
    while (true) {
        size_t child = 2 * root + 1; // left child
        if (child >= end) {
            break;
        }

        // Pick the larger child
        if (child + 1 < end
            && cmp(base + child * size, base + (child + 1) * size, arg) < 0) {
            child++;
        }

        // If root is already >= largest child, heap property holds
        if (cmp(base + root * size, base + child *  size, arg) >= 0) {
            break;
        }

        swap(base + root * size, base + child * size, size);
        root = child;
    }
}

void qsort_r(
    void* base,
    size_t n,
    size_t size,
    int (*compar)(const void* a, const void* b, void* arg),
    void* arg
) {
    if (n <= 2) {
        return;
    }
    char* arr = (char*)base;

    // Phase 1. Build max-heap
    for (size_t i = n / 2; i-- > 0; ) {
        sift_down(arr, i, n, size, compar, arg);
    }

    // Phase 2. Extract elements one by one
    for (size_t end = n - 1; end > 0; end--) {
        swap(arr, arr + end * size, size);
        sift_down(arr, 0, end, size, compar, arg);
    }
}
