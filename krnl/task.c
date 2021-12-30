#include "debug.h"
#include "cpu.h"
#include "kmalloc.h"
#include "types.h"
#include "task.h"
#include "pmm.h"
#include "vmm.h"
#include "idt.h"
#include "gdt.h"
#include "list.h"
#include "string.h"
#include "debug.h"
#include "intr_sync.h"

extern int init_task_func(void* arg);
extern int test_task(void* arg);

void initTask();
static Task_t* allocTask();
static char* setTaskName(Task_t* task, const char* name);
static char* getTaskName(Task_t* task);
static uint32_t getNewPid();
static void wakeupTask(Task_t* task);
static int32_t setupNewKstack(Task_t* task);

void kernel_thread_entry_s();
void fork_return_s(InterruptFrame_t* _if);
void switch_to_s(Context_t* from, Context_t* to);

int32_t createKernelThread(int (*func)(void*), void* arg, uint32_t clone_flags);
int32_t do_fork(uint32_t clone_flags, uint32_t stack, InterruptFrame_t* _if);
int32_t copy_mm(uint32_t clone_flags, Task_t* task);
int32_t copy_task(Task_t* task, uint32_t esp, InterruptFrame_t* _if);
void do_exit(int err_code);
static void forkret(void);


void initTask() {
    initList(&(TaskList.ptr));
    // 当前 main 函数的执行流即为系统首个内核进程, 
    // 在完成各个重要子系统初始化后变为系统空闲进程 idle
    // 剩下的初始化工作交给第二个进程 init 

    //构造首个进程 idle
    idle_task = allocTask();
    listAdd(&TaskList.ptr, &(idle_task->ptr));
    if (idle_task == NULL) {
        panic("Can not create idle task!");
    }
    idle_task->pid = 0;
    idle_task->state = TASK_RUNNABLE;
    idle_task->kstack = KernelStack;
    setTaskName(idle_task, "idle");

    TaskCount++;
    current = idle_task;

    int pid = createKernelThread(init_task_func, "Hello, world!", 0);

    if (pid < 0) {
        panic("create init process failed.");
    }
    int pid2 = createKernelThread(test_task, "Hello, world!", 0);
    //printk("%d\n", TaskCount);;
    
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
static int32_t setupNewKstack(Task_t* task) {
    uint32_t stack = (uint32_t) kmalloc(KSTACKSIZE);
    if (stack != NULL) {
        task->kstack = (uint32_t*) stack;
        return 0;
    }
    return ERR_NO_MEM;
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
void wakeupTask(Task_t* task) {
    task->state = TASK_RUNNABLE;
}
uint32_t getNewPid() {
    static uint32_t newestPid = 0;

    newestPid = TaskCount - 1;
    newestPid++;
    return newestPid;
}

//建立一个执行函数func的内核线程
int32_t createKernelThread(int (*func)(void*), void* arg, uint32_t clone_flags) {
    // 保存内核线程的临时中断帧

    InterruptFrame_t interruptFrame;
    memset(&interruptFrame, 0, sizeof(InterruptFrame_t));
    interruptFrame.cs = KERNEL_CS;
    interruptFrame.ds = interruptFrame.es = interruptFrame.ss = KERNEL_DS;
    interruptFrame.ebx = (uint32_t) func;
    interruptFrame.edx = (uint32_t) arg;
    interruptFrame.eip = (uint32_t) kernel_thread_entry_s;
    //printk("Stage11\n");
    return do_fork(clone_flags | CLONE_VM, 0, &interruptFrame);
}
//创建一个子进程所需的数据结构
int32_t do_fork(uint32_t clone_flags, uint32_t stack, InterruptFrame_t* _if) {
    //printk("Stage11_fork\n");
    Task_t* task;
    if (TaskCount >= MAX_TASKS) {
        return ERR_NO_FREE_TASK;
    }
    task = allocTask();
    if (task == NULL) {
        return ERR_NO_MEM;
    }

    int ret;
    task->parent = current;
    if (setupNewKstack(task) != 0) {
        ret = ERR_NO_MEM;
        goto failed_and_clean_task;
    }
    if (copy_mm(clone_flags, task) != 0) {
        ret = ERR_NO_MEM;
        goto failed_and_clean_kstack;
    }
    copy_task(task, stack, _if);

    bool flag;
    intr_save(flag);
    {
        task->pid = getNewPid();
        ret = task->pid;
        listAdd(&TaskList.ptr, &(task->ptr));
        TaskCount++;
    }
    intr_restore(flag);

    wakeupTask(task);

out:
    return ret;
failed_and_clean_kstack:
    kfree(task->kstack, KSTACKSIZE);
failed_and_clean_task:
    kfree(task, sizeof(Task_t));
failed_out:
    return ret;
}

// 根据clone_flag标志复制或共享进程内存管理结构
int32_t copy_mm(uint32_t clone_flags, Task_t* task) {
    //TODO
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
    SetBitOne(&(task->_if->eflags), 9);//打开中断
    task->context.eip = forkret;
    task->context.esp = (uint32_t) task->_if;
}
static void forkret(void) {
    fork_return_s(current->_if);
}
//进程终止处理，本函数不能返回
void do_exit(int err_code) {
    //panic("process exit!!.");
    current->state = TASK_ZOMBIE;
    bool flag;
    intr_save(flag);
    {
        printk("process exit!\n");
    }
    intr_restore(flag);
    schedule();
}