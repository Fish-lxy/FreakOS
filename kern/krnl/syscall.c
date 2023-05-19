#include "syscall.h"
#include "console.h"
#include "cpu.h"
#include "debug.h"
#include "idt.h"
#include "stdarg.h"
#include "sysfile.h"
#include "string.h"
#include "task.h"
#include "vmm.h"
#include "fat_base.h"
#include "types.h"

void syscall(InterruptFrame_t *_if);
// 注册系统调用中断
void initSysCall() { intrHandlerRegister(T_SYSCALL, syscall); }

static int sys_exit(uint32_t arg[]) {
    int error_code = (int)arg[0];
    return do_exit(error_code);
}

static int sys_fork(uint32_t arg[]) {
    InterruptFrame_t *tf = CurrentTask->_if;
    uint32_t stack = tf->user_esp;
    return do_fork(0, stack, tf);
}

static int sys_wait(uint32_t arg[]) {
    int pid = (int)arg[0];
    int *store = (int *)arg[1];
    return do_wait(pid, store);
}
static int sys_exec(uint32_t arg[]) {
    const char *name = (const char *)arg[0];
    int argc = (int)arg[1];
    const char **argv = (const char **)arg[2];
    return do_execve(name, argc, argv);
}
static int sys_getpid(uint32_t arg[]) { return CurrentTask->tid; }
static int sys_putc(uint32_t arg[]) {
    int c = (int)arg[0];
    consolePutc(c);
    //sprint_cur_pgdir();
    return 0;
}
static int sys_open(uint32_t arg[]) {
    const char *path = (const char *)arg[0];
    uint32_t open_flags = (uint32_t)arg[1];
    return sysfile_open(path, open_flags);
}

static int sys_close(uint32_t arg[]) {
    int fd = (int)arg[0];
    return sysfile_close(fd);
}

static int sys_read(uint32_t arg[]) {
    int fd = (int)arg[0];
    void *base = (void *)arg[1];
    size_t len = (size_t)arg[2];
    return sysfile_read(fd, base, len);
}

static int sys_write(uint32_t arg[]) {
    int fd = (int)arg[0];
    void *base = (void *)arg[1];
    size_t len = (size_t)arg[2];
    return sysfile_write(fd, base, len);
}

static int sys_seek(uint32_t arg[]) {
    int fd = (int)arg[0];
    int32_t pos = (int32_t)arg[1];
    int whence = (int)arg[2];
    return sysfile_seek(fd, pos, whence);
}

static int sys_fstat(uint32_t arg[]) {
    int fd = (int)arg[0];
    struct stat *stat = (struct stat *)arg[1];
    return sysfile_fstat(fd, stat);
}

static int sys_fsync(uint32_t arg[]) {
    int fd = (int)arg[0];
    return sysfile_fsync(fd);
}


static int sys_temp_ls(uint32_t arg[]) {
    ls("0:/");
    return 0;
}

static int (*syscalls[])(uint32_t arg[]) = {
    [SYS_exit] sys_exit,   [SYS_fork] sys_fork,     [SYS_wait] sys_wait,
    [SYS_exec] sys_exec,   [SYS_getpid] sys_getpid, [SYS_putc] sys_putc,
    [SYS_open] sys_open,   [SYS_close] sys_close,   [SYS_read] sys_read,
    [SYS_write] sys_write, [SYS_seek] sys_seek,     [SYS_fstat] sys_fstat,
    [SYS_fsync] sys_fsync, [SYS_temp_ls] sys_temp_ls,
};

#define NUM_SYSCALLS ((sizeof(syscalls)) / (sizeof(syscalls[0])))

void syscall(InterruptFrame_t *_if) {

    InterruptFrame_t *tf = CurrentTask->_if;

    uint32_t arg[MAX_ARGS];
    int num = tf->eax;
    // sprintk("-----syscall----\n no:%d, tid = %d, taskname = %s.\n", num,
    //         CurrentTask->tid, CurrentTask->name);
    if (num >= 0 && num < NUM_SYSCALLS) {
        if (syscalls[num] != NULL) {
            arg[0] = tf->edx;
            arg[1] = tf->ecx;
            arg[2] = tf->ebx;
            arg[3] = tf->edi;
            arg[4] = tf->esi;
            tf->eax = syscalls[num](arg);
            return;
        }
    }
    sprintk("undefined syscall %d, tid = %d, taskname = %s.\n", num,
            CurrentTask->tid, CurrentTask->name);
    sprintk("num:%d %d %d %d %d %d\n;", num, arg[0], arg[1], arg[2], arg[3],
            arg[4]);
}

int test_call_syscall(int num, ...) {
    uint32_t a[MAX_ARGS];
    bzero(a, sizeof(MAX_ARGS));

    va_list ap;
    va_start(ap, num);

    int i, ret;
    for (i = 0; i < MAX_ARGS; i++) {
        a[i] = va_arg(ap, uint32_t);
    }
    sprintk("num:%d %d %d %d %d %d\n", num, a[0], a[1], a[2], a[3], a[4]);

    va_end(ap);

    asm volatile("int %1;"
                 : "=a"(ret)
                 : "i"(T_SYSCALL), "a"(num), "d"(a[0]), "c"(a[1]), "b"(a[2]),
                   "D"(a[3]), "S"(a[4])
                 : "cc", "memory");
    return ret;
}

void test_syscall() {
    sprintk("---SYSCALL TEST---");
    test_call_syscall(SYS_putc, 's');
}
void test_putc_syscall(char *c) { test_call_syscall(SYS_putc, c); }
