#include "syscall.h"
#include "freakos.h"
#include "stdarg.h"
#include "types.h"

#define MAX_ARGS 5

int syscall(int num, ...) {
    va_list ap;
    va_start(ap, num);
    uint32_t a[MAX_ARGS];
    int i, ret;
    for (i = 0; i < MAX_ARGS; i++) {
        a[i] = va_arg(ap, uint32_t);
    }
    va_end(ap);
    // 事实上构造了 trapframe 的
    asm volatile("int %1;"
                 : "=a"(ret)       // 输出约束:返回值写入到 eax
                 : "i"(T_SYSCALL), // 输入操作数: 系统调用导致的中断
                   "a"(num),       // eax, 中断号
                   "d"(a[0]),      // edx
                   "c"(a[1]),      // ecx
                   "b"(a[2]),      // ebx
                   "D"(a[3]),      // edi
                   "S"(a[4])       // esi
                 : "cc", "memory"); // 乱码列表: 1.可能影响标志位 2.内存可能改变
    return ret;
}

int sys_exit(int error_code) {
    //
    return syscall(SYS_exit, error_code);
}
int sys_fork(void) {
    //
    return syscall(SYS_fork);
}
int sys_wait(int pid, int *store) {
    //
    return syscall(SYS_wait, pid, store);
}
int sys_getpid(void) {
    //
    return syscall(SYS_getpid);
}
int sys_putc(int c) {
    //
    return syscall(SYS_putc, c);
}
int sys_exec(const char *name, int argc, const char **argv) {
    return syscall(SYS_exec, name, argc, argv);
}
int sys_open(const char *path, uint32_t open_flags) {
    return syscall(SYS_open, path, open_flags);
}

int sys_close(int fd) { return syscall(SYS_close, fd); }

int sys_read(int fd, void *base, size_t len) {
    return syscall(SYS_read, fd, base, len);
}

int sys_write(int fd, void *base, size_t len) {
    return syscall(SYS_write, fd, base, len);
}

int sys_seek(int fd, int32_t pos, int whence) {
    return syscall(SYS_seek, fd, pos, whence);
}

int sys_fstat(int fd, Stat_t *stat) {
    //
    return syscall(SYS_fstat, fd, stat);
}

int sys_fsync(int fd) {
    //
    return syscall(SYS_fsync, fd);
}