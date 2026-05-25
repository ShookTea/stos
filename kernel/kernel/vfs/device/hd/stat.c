#include "./hd.h"
#include "errno.h"
#include <string.h>

int hd_stat(const vfs_node_t* node, struct stat* stat)
{
    if (node == NULL) {
        return -EIO;
    }
    memset(stat, 0, sizeof(struct stat));

    stat->st_size = node->length;
    return 0;
}
