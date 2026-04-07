#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sched.h>
#include <sys/wait.h>

int main(void)
{
    printf("Testing exec\n");
    int childpid = fork();
    int currpid = getpid();
    printf("[%d] fork res = %d\n", currpid, childpid);
    if (childpid == 0) {
        printf("\n[%d] running exec\n", currpid);
        puts("running exec");
        // In child task - run forking.c for demo
        exec("/initrd/forking");
        printf("\n[%d] ALERT: this line should never be printed.\n", currpid);
        return 1;
    } else {
        puts("Waiting for child to complete");
        int status = 0;
        waitpid(childpid, &status, 0);
        printf("\n[%d] Exec task completed with status %d\n", currpid, status);
        return 0;
    }
}
