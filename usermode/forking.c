#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(void)
{
    puts("Testing forking");
    int pid = fork();
    int currpid = getpid();
    printf("currpid = %d, fork res = %d\n", currpid, pid);
    puts("testing completed");

    return 0;
}
