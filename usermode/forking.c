#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sched.h>

int main(void)
{
    int currpid = getpid();
    printf("[%d] Testing forking\n", currpid);
    int pid = fork();
    printf("[%d] fork res = %d\n", currpid, pid);
    printf("[%d] Entering loop\n", currpid);
    for (int i = 0; i < 1000; i++) {
        for (int j = 0; j < 1000; j++) {
            volatile int x __attribute__((unused)) = i * j;
        }
    }
    printf("\n[%d] Testing completed\n", currpid);

    return 0;
}
