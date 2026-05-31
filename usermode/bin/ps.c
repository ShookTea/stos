#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STAT_BUFFER_LEN 256

int main(void)
{
    DIR* proc = opendir("/proc");
    if (proc == NULL) {
        return 1;
    }
    int status = 0;
    struct dirent* res = NULL;
    printf("%10s %10s %10s %-8s %-65s\n", "PID", "PPID", "PGID", "STATUS", "NAME");
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
            int pid = atoi(strtok(buffer, " "));
            char* procname = strtok(NULL, " ") + 1;
            procname[strlen(procname) - 1] = '\0';
            char status_char = strtok(NULL, " ")[0];
            int ppid = atoi(strtok(NULL, " "));
            int pgid = atoi(strtok(NULL, " "));

            char* status_label;
            switch (status_char) {
                case 'P': status_label = "waiting"; break;
                case 'R': status_label = "running"; break;
                case 'D': status_label = "blocked"; break;
                case 'S': status_label = "sleeping"; break;
                case 'Z': status_label = "zombie"; break;
                case 'X': status_label = "dead"; break;
                default: status_label = "???";
            }

            printf(
                "%10u %10u %10u %-8s %-65s\n",
                pid,
                ppid,
                pgid,
                status_label,
                procname
            );
        }
    }

    closedir(proc);
    return status;
}
