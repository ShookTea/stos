#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

int main(void)
{
    puts("Hello from userspace!");
    struct timespec* sleep = malloc(sizeof(struct timespec));
    sleep->tv_spec = 5;
    sleep->tv_nsec = 0;
    puts("Before sleep");
    nanosleep(sleep, NULL);
    puts("After sleep");

    return 0;
}
