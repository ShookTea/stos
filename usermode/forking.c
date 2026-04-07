#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sched.h>
#include <sys/wait.h>

int main(void)
{
    printf("Testing forking\n");
    int childpid = fork();
    int currpid = getpid();
    printf("[%d] fork res = %d\n", currpid, childpid);
    printf("[%d] Entering loop\n", currpid);
    for (int i = 0; i < 10000; i++) {
        for (int j = 0; j < 10000; j++) {
            volatile int x __attribute__((unused)) = i * j;
        }
    }

    if (childpid != 0) {
        printf("\n[%d] Waiting for child process to complete\n", currpid);
        int status = 0;
        waitpid(childpid, &status, 0);
        printf("\n[%d] Child task completed with status %d\n", currpid, status);
    }

    printf("\n[%d] Testing completed\n", currpid);

    return childpid == 0 ? 15 : 0;
}
