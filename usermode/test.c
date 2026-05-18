#include <errno.h>
#include <fcntl.h>
#include <string.h>
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
    int file = open(path, O_WRONLY);

    if (file == -1) {
        switch (errno) {
            case ENOENT: puts("Given path doesn't exist"); break;
            case EIO: puts("I/O error"); break;
            default: puts("Unrecognized error");
        }
        return 2;
    }

    char* text = "Hello, world!\n";
    int res = dprintf(file, text);
    int expected = strlen(text);

    printf("Printed %u characters, %u expected, error: %u\n", res, expected, errno);

    close(file);

    return res == expected ? 0 : 3;
}
