#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sched.h>

int main(void)
{
    printf("Testing forking\n");
    int pid = fork();
    int currpid = getpid();
    printf("[%d] fork res = %d\n", currpid, pid);
    printf("[%d] Entering loop\n", currpid);
    for (int i = 0; i < 10000; i++) {
        for (int j = 0; j < 10000; j++) {
            volatile int x __attribute__((unused)) = i * j;
        }
    }
    printf("\n[%d] Testing completed\n", currpid);
    if (pid != 0) {
        // TODO: wait for child process
    }

    return 0;
}
