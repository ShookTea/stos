#include <string.h>
#include <unistd.h>
#include <stdio.h>

#define LINE_BUF 4096

int main(int argc, char** argv)
{
    if (argc < 2) {
        dprintf(2, "Usage: grep <pattern>\n");
        return 1;
    }

    char* pattern = argv[1];
    char line[LINE_BUF];
    int len = 0;
    char c;

    while (read(0, &c, 1) == 1) {
        if (len < LINE_BUF - 1) {
            line[len++] = c;
        }
        if (c == '\n' || len == LINE_BUF - 1) {
            line[len] = '\0';
            if (strstr(line, pattern) != NULL) {
                write(1, line, len);
            }
            len = 0;
        }
    }

    if (len > 0) {
        line[len] = '\0';
        if (strstr(line, pattern) != NULL) {
            write(1, line, len);
        }
    }

    return 0;
}
