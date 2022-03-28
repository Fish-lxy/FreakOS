#ifndef TASK_H
#define TASK_H

#include "types.h"
#include "pmm.h"
#include "vmm.h"
#include "idt.h"
#include "list.h"

/* fork flags used in do_fork*/
#define CLONE_VM            0x00000100  // set if VM shared between processes
#define CLONE_THREAD        0x00000200  // thread group

#define MAX_TASKS 4096

#define ERR_NO_FREE_TASK -1
#define ERR_NO_MEM -2

#define KSTACKSIZE 8192

typedef enum TaskState_e {
    TASK_UNINIT = 0,
    TASK_SLEEPING = 1,
    TASK_RUNNABLE = 2,
    TASK_ZOMBIE = 3
}TaskState_e;
// typedef enum task_state_t TaskState_e;

typedef
struct Context_t {
    uint32_t eip;
    uint32_t esp;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
}Context_t;

#define TASK_NAME_LEN 15

typedef struct Task_t Task_t;
typedef
struct Task_t {
    int32_t pid;
    char name[TASK_NAME_LEN + 1];
    TaskState_e state;
    int runs;
    uint32_t* kstack;

    uint32_t flags;
    Task_t* parent;
    mm_struct_t* mm;
    Context_t context;
    InterruptFrame_t* _if;
    uint32_t cr3;

    list_ptr_t ptr;
}Task_t;

typedef
struct TaskList_t {
    list_ptr_t ptr;
}TaskList_t;

#define lp2task(lp, member)                 \
    to_struct((lp), Task_t, member)


extern uint8_t KernelStack[];

extern uint32_t kernel_cr3; // mm/vmm.c

void schedule();

#endif