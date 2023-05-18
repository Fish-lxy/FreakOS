#ifndef __TASK_H
#define __TASK_H

#include "idt.h"
#include "list.h"
#include "pmm.h"
#include "types.h"
#include "vmm.h"
#include "file.h"


#define EXEC_MAX_ARG_NUM    32
#define EXEC_MAX_ARG_LEN    4095

/* fork flags used in do_fork*/
#define CLONE_VM 0x00000100     // set if VM shared between processes
#define CLONE_THREAD 0x00000200 // thread group
#define CLONE_FILES 0x00000800

#define MAX_TASKS 4096

#define ERR_NO_FREE_TASK -1
#define ERR_NO_MEM -2



typedef enum TaskState_e {
    TASK_UNINIT = 0,
    TASK_SLEEPING = 1,
    TASK_RUNNABLE = 2,
    TASK_ZOMBIE = 3
} TaskState_e;
// typedef enum task_state_t TaskState_e;

// 等待状态(等待原因)
#define WT_NOTWAIT 0                           // 未等待
#define WT_INTERRUPTED 0x80000000              // 可中断的等待
#define WT_CHILD (0x00000001 | WT_INTERRUPTED) // 等待子进程
#define WT_KSEM 0x00000100 // 等待内核信号量，不可中断
#define WT_TIMER (0x00000002 | WT_INTERRUPTED) // 等待定时器
#define WT_KBD (0x00000004 | WT_INTERRUPTED)   // 等待键盘输入

typedef struct Context_t {
    uint32_t eip;
    uint32_t esp;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
} Context_t;

#define TASK_NAME_LEN (31)

typedef struct Task_t Task_t;
typedef struct Task_t {
    int32_t tid;
    char name[TASK_NAME_LEN + 1];

    TaskState_e state;
    uint32_t wait_state;
    int exit_code;

    int runs;
    uint32_t *kstack;

    uint32_t flags;
    Task_t *parent;
    MemMap_t *mm;
    Context_t context;
    InterruptFrame_t *_if;
    uint32_t cr3;

    uint32_t time_slice;
    volatile bool need_resched;
    list_ptr_t run_queue_ptr;

    FileStruct_t* files;

    list_ptr_t ptr;
} Task_t;

typedef struct TaskList_t {
    list_ptr_t ptr;
} TaskList_t;

#define lp2task(lp, member) to_struct((lp), Task_t, member)

extern uint8_t KernelStack[];

extern uint32_t kernel_cr3; // mm/vmm.c

extern Task_t *idle_task;
extern Task_t *CurrentTask;

extern uint32_t TaskCount;


void initTask();
int32_t createKernelThread(int (*func)(void*), void* arg, uint32_t clone_flags);

void do_exit(int err_code);
int do_execve(const char *name, int argc, const char **argv);
#endif