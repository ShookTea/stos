#include <stdio.h>
#include <time.h>

int main(void)
{
    puts("Hello from userspace!");
    time_t ts = time(NULL);
    printf("timestamp = %dll\n", ts);
    return 0;
}
