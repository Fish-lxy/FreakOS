#include "ulib.h"
#include "stdarg.h"
#include "string.h"

extern int vsprintf(char *buf, const char *fmt, va_list args);

void printf(const char *format, ...) {
    static char buf[512];

    va_list args;
    int i;

    va_start(args, format);;
    i = vsprintf(buf, format, args);
    va_end(args);

    buf[i] = '\0';

    int str_len = strlen(buf);

    write(STDOUT,buf,str_len);
    return;
}