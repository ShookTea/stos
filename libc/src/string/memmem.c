#include <string.h>

/**
 * Compute the critical factorization of the needle.
    Returns the period and sets *period_out.
 */
static size_t critical_factorization(
    const unsigned char* n,
    size_t nl,
    size_t* period_out
) {
    size_t ms = 0, p = 1; // max suffix position, period
    size_t j = 0, k = 1;

    // Maximal suffix (lexicographic)
    while (j + k < nl) {
        if (n[j + k] == n[ms + k]) {
            if (k == p) {
                j += p; k = 1;
            }
            else {
                k++;
            }
        } else if (n[j + k] > n[ms + k]) {
            j += k;
            k = 1;
            p = j - ms;
        } else {
            ms = j;
            j = ms + 1;
            k = 1;
            p = 1;
        }
    }
    *period_out = p;
    return ms;
}

void* memmem(
    const void* haystack,
    size_t haystacklen,
    const void* needle,
    size_t needlelen
) {
    if (needlelen == 0) {
        return (void*) haystack;
    }

    if (needlelen > haystacklen) {
        return NULL;
    }

    const unsigned char* h = (const unsigned char*)haystack;
    const unsigned char* n = (const unsigned char*)needle;

    // Fall back to naive for small needles — overhead not worth it
    if (needlelen < 8) {
        size_t limit = haystacklen - needlelen;
        for (size_t i = 0; i <= limit; i++) {
            if (h[i] == n[0] && memcmp(h + i, n, needlelen) == 0) {
                return (void*)(h + i);
            }
        }
        return NULL;
    }

    size_t period;
    size_t ms = critical_factorization(n, needlelen, &period);

    // Check if left and right halves overlap in their period
    int periodic = (memcmp(n, n + period, ms + 1) == 0);

    size_t j = 0;
    size_t memory = (size_t)-1; // last matched prefix length

    while (j + needlelen <= haystacklen) {
        size_t i = (ms > memory ? ms : memory) + 1;  // skip already-matched

        while (i < needlelen && n[i] == h[j + i]) {
            i++;
        }

        if (i < needlelen) {
            // Mismatch in right half — advance by period or jump
            j += (i - ms);
            if (periodic) {
                memory = needlelen - period - 1;
            }
            else {
                memory = (size_t)-1;
            }
        } else {
            // Right half matched — check left half
            size_t left = (memory == (size_t)-1) ? 0 : memory + 1;
            i = ms + 1;
            while (i > left && n[i - 1] == h[j + i - 1]) {
                i--;
            }

            if (i <= left) {
                return (void*)(h + j);  // Full match
            }

            j += period;
            if (periodic) {
                memory = needlelen - period - 1;
            }
            else {
                memory = (size_t)-1;
            }
        }
    }

    return NULL;
}
