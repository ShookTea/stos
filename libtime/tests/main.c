#include "runner.h"
#include <stdio.h>

int _failures = 0;
int _total = 0;

void test_days_since_epoch(void);
void test_datetime_to_timestamp(void);

int main(void)
{
    printf("days_since_epoch\n");
    test_days_since_epoch();
    printf("datetime_to_timestamp\n");
    test_datetime_to_timestamp();

    printf("\n%d passed, %d failed\n", _total - _failures, _failures);
    return _failures > 0 ? 1 : 0;
}
