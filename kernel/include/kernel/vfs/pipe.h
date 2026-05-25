#ifndef INCLUDE_KERNEL_VFS_PIPE_H
#define INCLUDE_KERNEL_VFS_PIPE_H

/**
 * Create a new pipe and makes it owned by current process. Stores file
 * descriptors in `read_fd` and `write_fd`.
 *
 * On success, returns 0. On failure, returns negation of one of errno values.
 *
 * If write side of the pipe is closed, then reading from read side will return
 * EOF. If read side of the pipe is closed, then writing to the write side will
 * return error EPIPE. When both are closed, the pipe is deallocated.
 */
int pipe_create(int* read_fd, int* write_fd, int flags);

#endif
