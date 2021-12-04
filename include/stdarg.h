#ifndef STDARG_H
#define STDARG_H

typedef __builtin_va_list va_list;

#define va_start(ap, last)         (__builtin_va_start(ap, last))//gcc提供
#define va_arg(ap, type)           (__builtin_va_arg(ap, type))
#define va_end(ap) 

#endif