#include "types.h"
#include "task.h"
#include "pmm.h"
#include "vmm.h"
#include "idt.h"
#include "gdt.h"
#include "list.h"
#include "string.h"
#include "debug.h"

extern uint8_t KernelStack[];
static Task_t* current;
static Task_t* idle_task;
static Task_t* init_task;
list_ptr_t TaskList;
uint32_t TaskCount = 0;
extern uint32_t kernel_cr3;

void taskInit();
Task_t* allocTask();
char* setTaskName(Task_t* task, const char* name);
char* getTaskName(Task_t* task);
uint32_t getPid();
static int32_t setupNewKstack(Task_t* task);

void kernel_thread_entry_s();
int32_t kernel_thread(int (*func)(void*), void* arg, uint32_t clone_flags);
int32_t do_fork(uint32_t clone_flags, uint32_t stack, InterruptFrame_t* _if);
int32_t copy_mm(uint32_t clone_flags, Task_t* task);
int32_t copy_task(Task_t* task, uint32_t esp, InterruptFrame_t* _if);
void do_exit(int err_code);
static void forkret(void);

void taskInit() {
    initList(&TaskList);
    if (idle_task = allocTask() == NULL) {
        panic("Can not create idle task!");
    }
    idle_task->pid = 0;
    idle_task->state = TASK_RUNNABLE;
    idle_task->kstack = KernelStack;
    idle_task->need_resched = 1;

}
Task_t* allocTask() {
    Task_t* task = (Task_t*) kmalloc(sizeof(Task_t));
    if (task == NULL) {
        return NULL;
    }
    if (task != NULL) {
        task->state = TASK_UNINIT;
        task->pid = -1;
        task->runs = 0;
        task->kstack = 0;
        task->need_resched = 0;
        task->parent = NULL;
        task->mm = NULL;
        memset(&(task->context), 0, sizeof(Context_t));
        task->_if = NULL;
        task->cr3 = kernel_cr3;
        task->flags = 0;
        memset(task->name, 0, TASK_NAME_LEN);
        return task;
    }

}
char* setTaskName(Task_t* task, const char* name) {
    memset(task->name, 0, sizeof(task->name));
    return memcpy(task->name, name, TASK_NAME_LEN);
}
char* getTaskName(Task_t* task) {
    static char name[TASK_NAME_LEN + 1];
    memset(name, 0, sizeof(name));
    return memcpy(name, task->name, TASK_NAME_LEN);
}
uint32_t getPid() {
    static uint32_t newestPid = 0;

    newestPid++;
    return newestPid;
}

//建立一个执行函数func的内核线程
int32_t kernel_thread(int (*func)(void*), void* arg, uint32_t clone_flags) {
    // 保存内核线程的临时中断帧
    InterruptFrame_t interruptFrame;
    memset(&interruptFrame, 0, sizeof(InterruptFrame_t));
    interruptFrame.cs = KERNEL_CS;
    interruptFrame.ds = interruptFrame.es = interruptFrame.ss = KERNEL_DS;
    interruptFrame.ebx = (uint32_t) func;
    interruptFrame.edx = (uint32_t) arg;
    interruptFrame.eip = (uint32_t) kernel_thread_entry_s;
    return do_fork(clone_flags | CLONE_VM, 0, &interruptFrame);
}
//创建一个子进程
int32_t do_fork(uint32_t clone_flags, uint32_t stack, InterruptFrame_t* _if) {
    Task_t* task;
    if (TaskCount >= MAX_TASKS) {
        return ERR_NO_FREE_TASK;
    }
    if (task = allocTask() == NULL) {
        return ERR_NO_MEM;
    }
    int ret;
    task->parent = current;
    if (setupNewKstack(task) != 0) {
        goto failed_and_clean_task;
    }
    if (copy_mm(clone_flags, task) != 0) {
        goto failed_and_clean_kstack;
    }
    copy_task(task, stack, _if);

    task->pid = getPid();
    listAdd(&TaskList, &(task->ptr));
    TaskCount++;

out:
    return ret;
failed_and_clean_kstack:
    kfree(task->kstack, KSTACKSIZE);
failed_and_clean_task:
    kfree(task, sizeof(Task_t));
failed_out:
    return ret;
}
static int32_t setupNewKstack(Task_t* task) {
    uint32_t stack = (uint32_t) kmalloc(KSTACKSIZE);
    if (stack != NULL) {
        task->kstack = (uint32_t*) stack;
        return 0;
    }
    return ERR_NO_MEM;
}
// 根据clone_flag标志复制或共享进程内存管理结构
int32_t copy_mm(uint32_t clone_flags, Task_t* task) {//TODO
    assert(current->mm == NULL, "mm is not NULL!");
    return 0;
}
//设置进程在内核（用户态）正常运行和调度所需的中断帧和执行上下文
//在进程的内核栈顶设置中断帧
//设置进程的内核进入点和进程栈
int32_t copy_task(Task_t* task, uint32_t esp, InterruptFrame_t* _if) {
    task->_if = (InterruptFrame_t*) (task->kstack + KSTACKSIZE) - 1;
    *(task->_if) = *_if;
    task->_if->eax = 0;
    task->_if->esp = esp;
    task->_if->eflags = SetBitOne(task->_if->eflags, 9);//打开中断
    task->context.eip = forkret;
    task->context.esp = (uint32_t) task->_if;
}
static void forkret(void) {
    //forkrets(current->_if);
}
void do_exit(int err_code) {
    panic("process exit!!.\n");
}