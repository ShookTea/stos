#include "runner.h"
#include <stdio.h>

int _failures = 0;
int _total = 0;

void test_days_since_epoch(void);
void test_datetime_to_timestamp(void);
void test_timestamp_to_datetime(void);
void test_weekday_from_days(void);

int main(void)
{
    puts("days_since_epoch");
    test_days_since_epoch();
    puts("datetime_to_timestamp");
    test_datetime_to_timestamp();
    puts("timestamp_to_datetime");
    test_timestamp_to_datetime();
    puts("weekday_from_days");
    test_weekday_from_days();

    printf("\n%d passed, %d failed\n", _total - _failures, _failures);
    return _failures > 0 ? 1 : 0;
}
