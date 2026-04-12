#if !(defined(__is_libk))

#include <sys/syscall.h>
#include <unistd.h>
#include <stdlib.h>

#define DIR_NAME_LENGTH 4096

char* get_current_dir_name(void)
{
    char* path = malloc(DIR_NAME_LENGTH);
    if (getcwd(path, DIR_NAME_LENGTH) == NULL) {
        free(path);
        return NULL;
    }
    return path;
}

#endif
