#include "runner.h"
#include <stdlib.h>
#include <libds/libds.h>
#include <stdio.h>

int _failures = 0;
int _total = 0;

void test_ringbuf(void);

int main(void)
{
    libds_set_allocators(malloc, realloc, free);

    printf("ringbuf\n");
    test_ringbuf();

    printf("\n%d passed, %d failed\n", _total - _failures, _failures);
    return _failures > 0 ? 1 : 0;
}
