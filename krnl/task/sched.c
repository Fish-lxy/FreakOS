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

void schedule();
static void runTask(Task_t* next_task);

extern Task_t* CurrentTask;
extern TaskList_t TaskList; //任务链表首节点
extern uint32_t TaskCount;

void schedule() {
    bool intrflag;
    list_ptr_t* now, * temp;
    Task_t* next = NULL;
    Task_t* task = NULL;
    intr_save(intrflag);
    {
        now = &(CurrentTask->ptr);
        temp = now;
        while (1) {
            temp = listGetNext(temp);
            if (temp == &(TaskList.ptr)) {
                continue;
            }

            task = lp2task(temp, ptr);
            if (TaskCount != 1 && task->pid == 0) {
                continue;
            }
            if (task->state != TASK_RUNNABLE) {
                continue;
            }
            else {
                next = task;
                break;
            }
        }
        runTask(next);
    }
    intr_restore(intrflag);
}
void runTask(Task_t* next_task) {
    if (next_task != CurrentTask) {
        Task_t* prev = CurrentTask;
        Task_t* next = next_task;

        bool flag;
        intr_save(flag);
        {
            CurrentTask = next_task;
            load_esp0(next->kstack + KSTACKSIZE);
            lcr3(next->cr3);
            switch_to_s(&(prev->context), &(next->context));
        }
        intr_restore(flag)
    }
}