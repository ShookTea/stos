#if !(defined(__is_libk))

#include <unistd.h>
#include "./_stdio_format.h"

static int emit_char(__stdio_format_out_t* out, char c)
{
    if (out->output.fd.len == STDIO_BUF_SIZE || c == 0) {
        write(out->output.fd.fd, out->output.fd.buf, out->output.fd.len);
        out->output.fd.len = 0;
    }

    if (c != 0) {
        out->output.fd.buf[out->output.fd.len] = c;
        out->output.fd.len++;
        return 1;
    }

    return 0;
}

int vdprintf(int fd, const char* restrict format, va_list list)
{
    __stdio_format_out_t out;
    out.emit_char = emit_char;
    out.output.fd.fd = fd;
    out.output.fd.len = 0;
    int res = __stdio_format_core(&out, format, list);
    emit_char(&out, 0);
    return res;
}

#endif
