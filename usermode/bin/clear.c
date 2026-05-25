#include <errno.h>
#include <stdio.h>

int main(void)
{
    errno = 0;
    FILE* tty = fopen("/dev/tty", "w");
    if (tty == NULL) {
        return errno == 0 ? 1 : errno;
    }
    if (fprintf(tty, "\033[2J\033[H") == EOF) {
        return errno == 0 ? 1 : errno;
    }

    if (fclose(tty) == EOF) {
        return errno == 0 ? 1 : errno;
    }

    return 0;
}
