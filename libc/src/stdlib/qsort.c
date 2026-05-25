#include <stdlib.h>

static int compar_for_qsort_r(const void* a, const void* b, void* orig_compar)
{
    int (*origin)(const void* a, const void* b) = orig_compar;
    return origin(a, b);
}

void qsort(
    void* base,
    size_t n,
    size_t size,
    int (*compar)(const void* a, const void* b)
) {
    qsort_r(base, n, size, compar_for_qsort_r, compar);
}
