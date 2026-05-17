#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

int main(int argc, char** argv)
{
    if (argc < 3) {
        puts("'write' requires at least 2 arguments (path and content)");
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

    int total_buf_size = 0;
    for (int i = 2; i < argc; i++) {
        total_buf_size += strlen(argv[i]) + 1;
    }
    char* buf = malloc(sizeof(char) * total_buf_size);
    int loc = 0;
    for (int i = 2; i < argc; i++) {
        int len = strlen(argv[i]);
        memcpy(buf + loc, argv[i], len);
        buf[loc + len] = (i == argc-1) ? '\0' : ' ';
        loc += (len + 1);
    }

    int res = write(file, buf, total_buf_size);
    printf("%u bytes written (%u expected)\n", res, total_buf_size);
    close(file);

    return res == total_buf_size ? 0 : 3;
}
