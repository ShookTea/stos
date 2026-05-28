#include <ctype.h>
#include <dirent.h>
#include <stdio.h>

#define STAT_BUFFER_LEN 256

int main(void)
{
    DIR* proc = opendir("/proc");
    if (proc == NULL) {
        return 1;
    }
    int status = 0;
    struct dirent* res = NULL;
    while ((res = readdir(proc)) != NULL) {
        if (isdigit(res->d_name[0])) {
            char stat_path[64] = { 0 };
            sprintf(stat_path, "/proc/%s/stat", res->d_name);
            FILE* file = fopen(stat_path, "r");
            if (file == NULL) {
                status = 1;
                continue;
            }
            char buffer[STAT_BUFFER_LEN] = {0};
            if (fgets(buffer, STAT_BUFFER_LEN, file) == NULL) {
                status = 1;
                continue;
            }
            printf("%s\n", buffer);
        }
    }

    closedir(proc);
    return status;
}
