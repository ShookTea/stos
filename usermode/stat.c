#include "sys/stat.h"
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

int main(int argc, char** argv)
{
    if (argc != 2) {
        puts("'stat' requires 2 arguments (path)");
        return 1;
    }

    char* path = argv[1];
    errno = 0;
    int file = open(path, O_RDONLY);

    if (file == -1) {
        switch (errno) {
            case ENOENT: puts("Given path doesn't exist"); break;
            case EIO: puts("I/O error"); break;
            default: puts("Unrecognized error");
        }
        return 2;
    }

    struct stat stat;
    if (fstat(file, &stat) != 0) {
        puts("fstat error");
        return 3;
    }

    printf("Creation time: %s", ctime(&stat.st_ctim.tv_spec));
    printf("Last access time: %s", ctime(&stat.st_atim.tv_spec));
    printf("Last modification time: %s", ctime(&stat.st_mtim.tv_spec));
    printf("Size: %u bytes\n", stat.st_size);
    close(file);

    return 0;
}
