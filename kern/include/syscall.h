#ifndef __SYSCALL_H_
#define __SYSCALL_H_

#define T_SYSCALL 0xAA

#define MAX_ARGS            5

#define SYS_exit            1
#define SYS_fork            2
#define SYS_wait            3
#define SYS_exec            4
#define SYS_clone           5
#define SYS_yield           10
#define SYS_sleep           11
#define SYS_kill            12
#define SYS_gettime         17
#define SYS_getpid          18
#define SYS_mmap            20
#define SYS_munmap          21
#define SYS_shmem           22
#define SYS_putc            30
#define SYS_pgdir           31
#define SYS_open            100
#define SYS_close           101
#define SYS_read            102
#define SYS_write           103
#define SYS_seek            104
#define SYS_fstat           110
#define SYS_fsync           111
#define SYS_getcwd          121
#define SYS_getdirentry     128
#define SYS_dup             130

struct InterruptFrame_t;
typedef struct InterruptFrame_t InterruptFrame_t;

void initSysCall();

void syscall(InterruptFrame_t *);
void test_syscall();
void test_putc_syscall(char *c);

#endif