#include "syscall.h"
#include "console.h"
#include "cpu.h"
#include "debug.h"
#include "idt.h"
#include "stdarg.h"
#include "string.h"
#include "task.h"
#include "types.h"

void syscall(InterruptFrame_t *_if);
// 注册系统调用中断
void initSysCall() { intrHandlerRegister(T_SYSCALL, syscall); }

static int sys_putc(uint32_t arg[]) {
    int c = (int)arg[0];
    consolePutc(c);
    return 0;
}
static int sys_exec(uint32_t arg[]) {
    const char *name = (const char *)arg[0];
    int argc = (int)arg[1];
    const char **argv = (const char **)arg[2];
    return do_execve(name, argc, argv);
}

static int (*syscalls[])(uint32_t arg[]) = {
    [SYS_exec] sys_exec,
    [SYS_putc] sys_putc,
};

#define NUM_SYSCALLS ((sizeof(syscalls)) / (sizeof(syscalls[0])))

void syscall(InterruptFrame_t *_if) {
    // TODO
    // InterruptFrame_t *tf = CurrentTask->_if;
    InterruptFrame_t *tf = _if;
    uint32_t arg[MAX_ARGS];
    int num = tf->eax;
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
