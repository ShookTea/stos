#ifndef _STDIO_H
#define _STDIO_H 1

#include <sys/cdefs.h>
#include <stdarg.h>
#include <stddef.h>

#define EOF (-1)
#define BUFSIZ 1024

#if !(defined(__is_libk))
/**
 * File stream structure.
 */
typedef struct {
    /** File descriptor ID (from fcntl.h) */
    int fd;
    /** Mode used for opening a file (combination of O_ flags from fcntl.h) */
    int mode;
    // Buffer for reading from stream
    char read_buf[BUFSIZ];
    int read_buf_pos;
    int read_buf_len;
    int ungetc_buf; // EOF means empty
} FILE;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Write a single character to the standard output stream. If successful,
 * returns the same character; otherwise returns EOF.
 */
int putchar(int);

/**
 * Writes a null-terminated string + a newline to stdout. Returns the number of
 * bytes written (including the newline).
 *
 * In case of error, EOF is returned and `errno` can be set to any value defined
 * in `printf`.
 */
int puts(const char*);

/**
 * Prints formatted string. Returns the number of characters written, or EOF in
 * case of error.
 *
 * Documentation in the sdtio/_stdio_format.c file gives more information about
 * format placeholders.
 *
 * In case of error, EOF is returned and `errno` can be set to any value defined
 * in `write`.
 */
int printf(const char* __restrict, ...);

/**
 * Prints formatted string using va_list argument. Returns the number of
 * characters written, or EOF in case of error.
 *
 * Documentation in the sdtio/_stdio_format.c file gives more information about
 * format placeholders.
 *
 * In case of error, EOF is returned and `errno` can be set to any value defined
 * in `write`.
 */
int vprintf(const char* restrict format, va_list list);

/**
 * Formats tring and puts the output into the buffer, followed by a NULL byte.
 * Returns the number of characters written, or EOF in case of error. It is
 * user's responsibility to ensure that enough space is available.
 *
 * Documentation in the sdtio/_stdio_format.c file gives more information about
 * format placeholders.
 *
 * In case of error, EOF is returned and `errno` can be set to any value defined
 * in `write`.
 */
int sprintf(char* restrict buffer, const char* restrict format, ...);

/**
 * Formats tring and puts the output into the buffer, followed by a NULL byte.
 * Returns the number of characters written, or EOF in case of error. It is
 * user's responsibility to ensure that enough space is available.
 *
 * Documentation in the sdtio/_stdio_format.c file gives more information about
 * format placeholders.
 *
 * In case of error, EOF is returned and `errno` can be set to any value defined
 * in `write`.
 */
int vsprintf(char* restrict buffer, const char* restrict format, va_list list);

/**
 * Formats tring and puts at most N-1 characters of output into the buffer,
 * followed by a NULL byte. Returns the number of characters written, or EOF in
 * case of error. If N is zero, then nothing will be written and buffer may be
 * a null pointer. It is user's responsibility to ensure that enough space is
 * available.
 *
 * Documentation in the sdtio/_stdio_format.c file gives more information about
 * format placeholders.
 *
 * In case of error, EOF is returned and `errno` can be set to any value defined
 * in `write`.
 */
int snprintf(char* restrict buffer, size_t n, const char* restrict format, ...);

/**
 * Formats tring and puts at most N-1 characters of output into the buffer,
 * followed by a NULL byte. Returns the number of characters written, or EOF in
 * case of error. If N is zero, then nothing will be written and buffer may be
 * a null pointer. It is user's responsibility to ensure that enough space is
 * available.
 *
 * Documentation in the sdtio/_stdio_format.c file gives more information about
 * format placeholders.
 *
 * In case of error, EOF is returned and `errno` can be set to any value defined
 * in `write`.
 */
int vsnprintf(
    char* restrict buffer,
    size_t n,
    const char* restrict format,
    va_list list
);

#if !(defined(__is_libk))

extern FILE* stdin;
extern FILE* stdout;
extern FILE* stderr;

/**
 * Opens a file at given path, with selected mode, and returns a filestream
 * object.
 *
 * Available modes are:
 * -  "r" - open the file for reading. The stream is positioned at the beginning
 *         of the file.
 * - "r+" - open the file for reading and writing. The stream is positioned at
 *          the beginning of the file.
 * -  "w" - truncate the file to zero length or create the file for writing. The
 *          stream is positioned at the beginning of the file.
 * - "w+" - open the file for reading and writing. The file is created if it
 *          doesn't exist, otherwise it's truncated. The stream is positioned
 *          at the beginning of the file.
 * -  "a" - open the file for appending (writing at the end of the file). The
 *          file is created if it doesn't exist.
 * - "a+" - open the file for reading and appending.
 *
 * On failure, it will return NULL and set `errno` to one of following values:
 * - EINVAL - if `mode` provided to the function is invalid
 * - any error that can be reported by `malloc` or `open`
 */
FILE* fopen(const char* restrict path, const char* restrict mode);

/**
 * Associates given file descriptor with a mode and returns stream.
 */
FILE* fdopen(int fd, const char* mode);

/**
 * For output streams, this function will force write of all buffered data for
 * the given output.
 *
 * For input streams, this function will discard any buffered data that has
 * been fetched from the file, but hasn't been consumed by the application.
 *
 * In both cases, it will return 0 after success or EOF on failure.
 *
 * On error it can set any of errors defined in `write`.
 *
 * TODO: if stream is NULL, it should flush all open output streams.
 */
int fflush(FILE* stream);

/**
 * Flush the stream and close the underlying file descriptor. On success returns
 * 0, on failure returns EOF.
 *
 * On error it can set any of errors defined in `close`, `write` or `fflush`.
 */
int fclose(FILE* stream);

/**
 * Prints formatted string to `stream`. Returns the number of characters
 * written, or EOF in case of error.
 *
 * Documentation in the sdtio/_stdio_format.c file gives more information about
 * format placeholders.
 *
 * In case of error, EOF is returned and `errno` can be set to any value defined
 * in `write`.
 */
int fprintf(FILE* restrict stream, const char* restrict format, ...);

/**
 * Prints formatted string to `stream` using va_list argument. Returns the
 * number of characters written, or EOF in case of error.
 *
 * Documentation in the sdtio/_stdio_format.c file gives more information about
 * format placeholders.
 *
 * In case of error, EOF is returned and `errno` can be set to any value defined
 * in `write`.
 */
int vfprintf(FILE* restrict stream, const char* restrict format, va_list list);

/**
 * Prints formatted string to file descriptor `fd`. Returns the number of
 * characters written, or EOF in case of error.
 *
 * Documentation in the sdtio/_stdio_format.c file gives more information about
 * format placeholders.
 *
 * In case of error, EOF is returned and `errno` can be set to any value defined
 * in `write`.
 */
int dprintf(int fd, const char* restrict format, ...);

/**
 * Prints formatted string to file descriptor `fd` using va_list argument.
 * Returns the number of characters written, or EOF in case of error.
 *
 * Documentation in the sdtio/_stdio_format.c file gives more information about
 * format placeholders.
 *
 * In case of error, EOF is returned and `errno` can be set to any value defined
 * in `write`.
 */
int vdprintf(int fd, const char* restrict format, va_list list);

/**
 * Reads the next character from stream and returns it as an `unsigned char`
 * cast to `int`, or EOF at the end of file or error.
 */
int fgetc(FILE* stream);

/**
 * This is an equivalent of fgetc(stdin).
 */
int getchar(void);

/**
 * Reads at most one less than `size` characters from stream and store it in
 * the buffer pointed to by `s`. Reading stops after EOF or new line. If a new
 * line is read, it is stored in the buffer as well. A terminating NULL byte
 * is stored in the last character in the buffer.
 */
char* fgets(char* restrict s, int size, FILE* restrict stream);

/**
 * Pushes character `c` back to the stream, where it's available for subsequent
 * read operations.
 */
int ungetc(int c, FILE* stream);

#endif // #if !(defined(__is_libk))

#ifdef __cplusplus
}
#endif

#endif
