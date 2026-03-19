#include <kernel/memory/kmalloc.h>
#include <kernel/multiboot2.h>
#include <kernel/paging.h>
#include <kernel/vfs/vfs.h>
#include <kernel/vfs/initrd.h>

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    vfs_node_t* parent;
    void* address;
} initrd_file_data_t;

typedef struct {
    vfs_node_t* parent;
    vfs_node_t** children;
    size_t children_count;
} initrd_directory_data_t;

static vfs_node_t* initrd = NULL;
static vfs_node_t* all_initrd_files = NULL;
static bool mounted = false;
static char empty_name[100];
static size_t files_count = 0;

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

static vfs_node_t* create_new_file(
    char* filename,
    uint8_t type,
    vfs_node_t* parent
) {
    all_initrd_files = krealloc(
        all_initrd_files,
        sizeof(vfs_node_t) * (files_count + 1)
    );
    strcpy(all_initrd_files[files_count].filename, filename);
    all_initrd_files[files_count].type = type;
    all_initrd_files[files_count].length = 0;
    all_initrd_files[files_count].open_node = NULL;
    all_initrd_files[files_count].close_node = NULL;
    all_initrd_files[files_count].read_node = NULL;
    all_initrd_files[files_count].write_node = NULL;
    all_initrd_files[files_count].readdir_node = NULL;
    all_initrd_files[files_count].finddir_node = NULL;

    if (type & VFS_TYPE_FILE) {
        initrd_file_data_t* data = kmalloc(sizeof(initrd_file_data_t));
        all_initrd_files[files_count].metadata = data;
        data->parent = parent;
        data->address = NULL;
    } else if (type & VFS_TYPE_DIRECTORY) {
        initrd_directory_data_t* data =
            kmalloc(sizeof(initrd_directory_data_t));
        all_initrd_files[files_count].metadata = data;
        data->parent = parent;
        data->children = NULL;
        data->children_count = 0;
        all_initrd_files[files_count].readdir_node = initrd_readdir;
        all_initrd_files[files_count].finddir_node = initrd_finddir;
    }

    files_count++;
    return &(all_initrd_files[files_count - 1]);
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
            kmalloc_size(parts_buffer) + strlen(part)
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
    for (size_t i = 0; i < files_count; i++) {
        vfs_node_t entry = all_initrd_files[i];
        if (!(entry.type & VFS_TYPE_DIRECTORY)) {
            continue;
        }
        initrd_directory_data_t* dir_data = entry.metadata;
        if (dir_data->parent != parent) {
            continue;
        }
        if (strcmp(entry.filename, name) != 0) {
            continue;
        }
        return &(all_initrd_files[i]);
    }

    vfs_node_t* directory = create_new_file(name, VFS_TYPE_DIRECTORY, parent);
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
        VFS_TYPE_FILE,
        parent
    );
    new_file->length = size_bytes;

    kfree(parts);
    kfree(filename_buffer);
}

vfs_node_t* initrd_mount()
{
    if (mounted) {
        return initrd;
    }
    mounted = true;
    puts("Loading initrd from memory");
    memset(empty_name, 0, 100);

    multiboot_tag_boot_module_t* initrd_module = NULL;
    for (uint32_t i = 0; i < multiboot2_get_modules_count(); i++) {
        if (strcmp("initrd", multiboot2_get_boot_module_name(i)) == 0) {
            initrd_module = multiboot2_get_boot_module_entry(i);
            break;
        }
    }

    if (!initrd_module) {
        puts("There is no initrd module present.");
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
    data->parent = vfs_root;
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

void initrd_unmount()
{
    if (mounted && initrd) {
        for (size_t i = 0; i < files_count; i++) {
            vfs_node_t entry = all_initrd_files[i];
            if (entry.type & VFS_TYPE_DIRECTORY) {
                initrd_directory_data_t* metadata = entry.metadata;
                kfree(metadata->children);
            }
            kfree(entry.metadata);
        }

        kfree(all_initrd_files);

        initrd_directory_data_t* root_metadata = initrd->metadata;
        kfree(root_metadata->children);
        kfree(initrd->metadata);
        kfree(initrd);
    }
}
