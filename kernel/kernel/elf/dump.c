#include <kernel/memory/kmalloc.h>
#include <kernel/vfs/vfs.h>
#include <kernel/elf.h>

void elf_dump(vfs_file_t* handle)
{
    void* file = kmalloc_flags(handle->dentry->inode->length, KMALLOC_ZERO);
    vfs_read(handle, handle->dentry->inode->length, file);
    vfs_close(handle);
    // For now the validation code prints all the debug info
    elf_validate(file);
    kfree(file);
}
