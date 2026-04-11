#include <kernel/memory/kmalloc.h>
#include <kernel/multiboot2.h>
#include <kernel/paging.h>
#include <kernel/vfs/vfs.h>
#include <kernel/vfs/initrd.h>

#include "kernel/debug.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define _debug_puts(...) debug_puts_c("VFS/Initrd", __VA_ARGS__)
#define _debug_printf(...) debug_printf_c("VFS/Initrd", __VA_ARGS__)

typedef struct {
    void* address;
} initrd_file_data_t;

typedef struct {
    vfs_node_t** children;
    size_t children_count;
} initrd_directory_data_t;

static vfs_node_t* initrd = NULL;
static bool mounted = false;
static char empty_name[100];

static struct dirent* initrd_readdir(
    vfs_node_t* directory,
    size_t index
) {
    initrd_directory_data_t* dir_data = directory->metadata;
    if (index >= dir_data->children_count) {
        return NULL;
    }
    static struct dirent ent;
    strcpy(ent.name, dir_data->children[index]->filename);
    ent.ino = dir_data->children[index]->inode;
    return &ent;
}

static vfs_node_t* initrd_finddir(
    vfs_node_t* directory,
    char* name
) {
    initrd_directory_data_t* dir_data = directory->metadata;
    for (size_t i = 0; i < dir_data->children_count; i++) {
        if (strcmp(dir_data->children[i]->filename, name) == 0) {
            return dir_data->children[i];
        }
    }
    return NULL;
}

/**
 * Read content of given file, bytes from [offset] to [offset+size], and
 * store them in [ptr]. Return the number of read bytes.
 */
static size_t initrd_read(
    vfs_file_t* file,
    size_t offset,
    size_t size,
    void* ptr
) {
    initrd_file_data_t* metadata = file->node->metadata;
    void* file_start = metadata->address;
    void* file_end = metadata->address + file->node->length;

    void* read_start = file_start + offset;
    void* read_end = read_start + size;
    if (read_start < file_start) {
        read_start = file_start;
    }
    if (read_start > file_end) {
        read_start = file_end;
    }
    if (read_end < read_start) {
        read_end = read_start;
    }
    if (read_end > file_end) {
        read_end = file_end;
    }
    size_t read_size = read_end - read_start;
    memcpy(ptr, read_start, read_size);
    return read_size;
}

static vfs_node_t* create_new_file(
    char* filename,
    uint8_t type
) {
    vfs_node_t* new_node = kmalloc(sizeof(vfs_node_t));
    vfs_populate_node(new_node, filename, type);

    if (type & VFS_TYPE_DIRECTORY) {
        initrd_directory_data_t* data =
            kmalloc(sizeof(initrd_directory_data_t));
        new_node->metadata = data;
        data->children = NULL;
        data->children_count = 0;
        new_node->readdir_node = initrd_readdir;
        new_node->finddir_node = initrd_finddir;
    } else if (type & VFS_TYPE_FILE) {
        new_node->read_node = initrd_read;
    }

    return new_node;
}

/**
 * Splits filename by slash. Stores the total number of elements in `count`.
 *
 * !IMPORTANT returned value should be kfree'd after use.
 */
static char** split_path(char* filename, uint8_t* count)
{
    char** parts_buffer = NULL;
    *count = 0;
    char* part = strtok(filename, "/");
    do {
        parts_buffer = krealloc(
            parts_buffer,
            sizeof(char*) * (*count + 1)
        );
        parts_buffer[*count] = part;
        *count = *count + 1;
    } while ((part = strtok(NULL, "/")) != NULL);

    return parts_buffer;
}

/**
 * Tries to find among already loaded files a directory that is a child of
 * `parent` and has given name. If no such directory is found, creates a new
 * directory instead.
 */
static vfs_node_t* get_directory(vfs_node_t* parent, char* name)
{
    vfs_node_t* directory = initrd_finddir(parent, name);
    if (directory != NULL) {
        return directory;
    }

    directory = create_new_file(name, VFS_TYPE_DIRECTORY);

    initrd_directory_data_t* parent_metadata = parent->metadata;
    parent_metadata->children_count++;
    parent_metadata->children = krealloc(
        parent_metadata->children,
        sizeof(vfs_node_t*) * parent_metadata->children_count
    );
    parent_metadata->children[parent_metadata->children_count - 1] = directory;
    return directory;
}

/**
 * Iterates over file in TAR. Loads next TAR header to `next`.
 */
static void initrd_load_tar(tar_header_t* tar_header, tar_header_t** next)
{
    if (memcmp(tar_header->filename, empty_name, 100) == 0)
    {
        *next = NULL;
        return;
    }

    tar_header->filename[99] = '\0';
    uint32_t size_bytes = strtol(tar_header->size, NULL, 8);

    // Parsing filename
    char* filename_buffer = kmalloc(sizeof(char) * 100);
    strcpy(filename_buffer, tar_header->filename);
    uint8_t parts_count = 0;
    char** parts = split_path(filename_buffer, &parts_count);

    vfs_node_t* parent = initrd;

    // Looking for parent
    for (uint8_t i = 0; i < parts_count - 1; i++) {
        parent = get_directory(parent, parts[i]);
    }

    uint32_t size_aligned = size_bytes;
    if (size_aligned % 512 != 0) {
        size_aligned += 512 - (size_aligned % 512);
    }
    *next = (tar_header_t*)(((char*)tar_header) + 512 + size_aligned);

    vfs_node_t* new_file = create_new_file(
        parts[parts_count - 1],
        VFS_TYPE_FILE
    );
    new_file->length = size_bytes;
    initrd_file_data_t* file_metadata = kmalloc(sizeof(initrd_file_data_t));
    file_metadata->address = ((char*)tar_header) + 512;
    new_file->metadata = file_metadata;

    initrd_directory_data_t* parent_meta = parent->metadata;
    parent_meta->children = krealloc(
        parent_meta->children,
        sizeof(vfs_node_t*) * (parent_meta->children_count + 1)
    );
    parent_meta->children[parent_meta->children_count] = new_file;
    parent_meta->children_count++;


    kfree(parts);
    kfree(filename_buffer);
}

vfs_node_t* initrd_mount()
{
    if (mounted) {
        return initrd;
    }
    mounted = true;
    _debug_puts("Loading initrd from memory");
    memset(empty_name, 0, 100);

    multiboot_tag_boot_module_t* initrd_module = NULL;
    for (uint32_t i = 0; i < multiboot2_get_modules_count(); i++) {
        if (strcmp("initrd", multiboot2_get_boot_module_name(i)) == 0) {
            initrd_module = multiboot2_get_boot_module_entry(i);
            break;
        }
    }

    if (!initrd_module) {
        _debug_puts("There is no initrd module present.");
        return NULL;
    }

    initrd = kmalloc(sizeof(vfs_node_t));
    strcpy(initrd->filename, "initrd");
    initrd->type = VFS_TYPE_DIRECTORY | VFS_TYPE_MOUNTPOINT;
    initrd->open_node = NULL;
    initrd->close_node = NULL;
    initrd->read_node = NULL;
    initrd->write_node = NULL;
    initrd->readdir_node = initrd_readdir;
    initrd->finddir_node = initrd_finddir;
    initrd_directory_data_t* data = kmalloc(sizeof(initrd_directory_data_t));
    initrd->metadata = data;
    data->children = NULL;
    data->children_count = 0;

    tar_header_t* tar_header = PHYS_TO_VIRT(
        initrd_module->module_phys_addr_start
    );

    do {
        initrd_load_tar(tar_header, &tar_header);
    } while (tar_header != NULL);

    if (initrd) {
        vfs_mount_node(initrd);
    }

    return initrd;
}

static void free_node_recursive(vfs_node_t* node)
{
    if (node->type & VFS_TYPE_DIRECTORY) {
        initrd_directory_data_t* metadata = node->metadata;
        for (size_t i = 0; i < metadata->children_count; i++) {
            free_node_recursive(metadata->children[i]);
        }
        kfree(metadata->children);
        kfree(metadata);
    } else if (node->type & VFS_TYPE_FILE) {
        if (node->metadata != NULL) {
            kfree(node->metadata);
        }
    }
    kfree(node);
}

void initrd_unmount()
{
    if (mounted && initrd) {
        // TODO: call VFS and tell it that the node no longer exists
        free_node_recursive(initrd);
        initrd = NULL;
        mounted = false;
    }
}
