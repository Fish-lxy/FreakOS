#include "os/syscall.h"
#include "stdarg.h"
#include "types.h"
#include "freakos.h"

#define MAX_ARGS 5

static inline int syscall(int num, ...) {
    va_list ap;
    va_start(ap, num);
    uint32_t a[MAX_ARGS];
    int i, ret;
    for (i = 0; i < MAX_ARGS; i++) {
        a[i] = va_arg(ap, uint32_t);
    }
    va_end(ap);
    // 事实上构造了 trapframe 的
    asm volatile("int %1;"         // int =
                 : "=a"(ret)       // 输出约束:返回值写入到 eax
                 : "i"(T_SYSCALL), // 输入操作数: 系统调用导致的中断
                   "a"(num),       // eax, 中断号
                   "d"(a[0]),      // edx
                   "c"(a[1]),      // ecx
                   "b"(a[2]),      // ebx
                   "D"(a[3]),      // edi
                   "S"(a[4])       // esi
                 : "cc", "memory"); // 乱码列表: 1 可能影响标志位 2 内存可能改变
    return ret;
}

int sys_putc(int c) {
    return syscall(SYS_putc, c);
}