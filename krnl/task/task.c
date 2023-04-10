#include "task.h"
#include "cpu.h"
#include "debug.h"
#include "gdt.h"
#include "idt.h"
#include "intr_sync.h"
#include "kmalloc.h"
#include "list.h"
#include "pmm.h"
#include "sched.h"
#include "string.h"
#include "types.h"
#include "vmm.h"

extern int init_task_func(void *arg);
extern int test_task(void *arg);

void initTask();
static Task_t *allocTask();
static char *setTaskName(Task_t *task, const char *name);
static char *getTaskName(Task_t *task);
static uint32_t getNewPid();
static int32_t setupNewKstack(Task_t *task);
static void freeKstack(Task_t *task);

void kernel_thread_entry_s();
void fork_return_s(InterruptFrame_t *_if);
void switch_to_s(Context_t *from, Context_t *to);

int32_t createKernelThread(int (*func)(void *), void *arg,
                           uint32_t clone_flags);
int32_t do_fork(uint32_t clone_flags, uint32_t stack, InterruptFrame_t *_if);
int32_t copy_mm(uint32_t clone_flags, Task_t *task);
int32_t copy_task(Task_t *task, uint32_t esp, InterruptFrame_t *_if);
void do_exit(int err_code);
static void forkret(void);

Task_t *CurrentTask;
Task_t *idle_task;
Task_t *init_task;

TaskList_t TaskList; // 任务链表首节点
uint32_t TaskCount = 0;

void initTask() {

    initRunQueue();

    initList(&(TaskList.ptr));
    // 当前 main 函数的执行流即为系统首个内核进程,
    // 在完成各个子系统初始化后变为系统空闲进程 idle
    // 剩下的初始化工作交给第二个进程 init

    // 构造首个进程 idle
    idle_task = allocTask();
    listAdd(&TaskList.ptr, &(idle_task->ptr));
    if (idle_task == NULL) {
        panic("Can not create idle task!");
    }

    idle_task->tid = 0;
    idle_task->state = TASK_RUNNABLE;
    idle_task->wait_state = WT_NOTWAIT;
    idle_task->kstack = KernelStack;
    idle_task->need_resched = 1;
    setTaskName(idle_task, "idle");

    TaskCount++;
    CurrentTask = idle_task;

    int tid = createKernelThread(init_task_func, "Hello, world!", 0);

    if (tid < 0) {
        panic("create init process failed.");
    }
}
Task_t *allocTask() {
    Task_t *task = (Task_t *)kmalloc(sizeof(Task_t));

    if (task == NULL) {
        return NULL;
    }
    if (task != NULL) {
        task->state = TASK_UNINIT;
        task->wait_state = WT_NOTWAIT;
        task->time_slice = 0;
        task->need_resched = 0;
        task->tid = -1;
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
// 申请内核栈，TSS段将使用
static int32_t setupNewKstack(Task_t *task) {
    uint32_t stack = (uint32_t)kmalloc(KSTACKSIZE);
    if (stack != NULL) {
        task->kstack = (uint32_t *)stack;
        return 0;
    }
    return ERR_NO_MEM;
}
static void freeKstack(Task_t *task) { kfree(task->kstack, KSTACKSIZE); }
char *setTaskName(Task_t *task, const char *name) {
    memset(task->name, 0, sizeof(task->name));
    return memcpy(task->name, name, TASK_NAME_LEN);
}
char *getTaskName(Task_t *task) {
    static char name[TASK_NAME_LEN + 1];
    memset(name, 0, sizeof(name));
    return memcpy(name, task->name, TASK_NAME_LEN);
}

uint32_t getNewPid() {
    static uint32_t newestPid = 0;

    newestPid = TaskCount - 1;
    newestPid++;
    return newestPid;
}

// 建立一个执行函数func的内核线程
int32_t createKernelThread(int (*func)(void *), void *arg,
                           uint32_t clone_flags) {
    // 保存内核线程的临时中断帧

    InterruptFrame_t interruptFrame;

    memset(&interruptFrame, 0, sizeof(InterruptFrame_t));
    interruptFrame.cs = KERNEL_CS;
    interruptFrame.ds = interruptFrame.es = interruptFrame.ss = KERNEL_DS;
    interruptFrame.ebx = (uint32_t)func;
    interruptFrame.edx = (uint32_t)arg;
    interruptFrame.eip = (uint32_t)kernel_thread_entry_s;

    return do_fork(clone_flags | CLONE_VM, 0, &interruptFrame);
}
// 创建一个子进程所需的数据结构
int32_t do_fork(uint32_t clone_flags, uint32_t stack, InterruptFrame_t *_if) {
    // printk("Stage11_fork\n");
    Task_t *task;
    if (TaskCount >= MAX_TASKS) {
        return ERR_NO_FREE_TASK;
    }
    task = allocTask();
    if (task == NULL) {
        return ERR_NO_MEM;
    }

    int ret;
    task->parent = CurrentTask;
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
        task->tid = getNewPid();
        ret = task->tid;
        listAdd(&TaskList.ptr, &(task->ptr));
        TaskCount++;
    }
    intr_restore(flag);

    sprintk("新进程%d建立\n",ret);
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
int32_t copy_mm(uint32_t clone_flags, Task_t *task) {
    // TODO
    assert(CurrentTask->mm == NULL, "mm is not NULL!");
    return 0;
}
// 设置进程在内核（用户态）正常运行和调度所需的中断帧和执行上下文
// 在进程的内核栈顶设置中断帧
// 设置进程的内核进入点和进程栈
int32_t copy_task(Task_t *task, uint32_t esp, InterruptFrame_t *_if) {
    task->_if = (InterruptFrame_t *)(task->kstack + KSTACKSIZE) - 1;
    *(task->_if) = *_if;
    task->_if->eax = 0;
    task->_if->esp = esp;
    SetBit(&(task->_if->eflags), 9); // 打开中断
    task->context.eip = forkret;
    task->context.esp = (uint32_t)task->_if;
}
static void forkret(void) { fork_return_s(CurrentTask->_if); }
// 进程终止处理，本函数不能返回
void do_exit(int err_code) {
    if (CurrentTask == idle_task) {
        panic("Trying to exit idle!");
    }
    // panic("process exit!!.");
    if (CurrentTask->mm == NULL) { // 是内核进程
        CurrentTask->state = TASK_ZOMBIE;
        bool flag;
        intr_save(flag);
        { 
            //printk("Task %d:process exit!\n", CurrentTask->tid); 
        }
        intr_restore(flag);
    }
    // TODO 用户态进程退出
    sprintk("进程%d退出!\n", CurrentTask->tid);

    schedule();
    panic("do_exit will not return!");
}