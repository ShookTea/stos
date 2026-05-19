#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

int main(int argc, char** argv)
{
    if (argc != 2) {
        puts("'write' requires 1 argument (path)");
        return 1;
    }

    char* path = argv[1];
    errno = 0;
    char buf[128] = {0};

    int res = readlink(path, buf, 127);
    if (res < 0) {
        printf("Err %u\n", errno);
    } else {
        printf("Readlink res: '%s'\n", buf);
    }

    return res < 0 ? 2 : 0;
}
