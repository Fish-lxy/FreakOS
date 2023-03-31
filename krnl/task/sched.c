#include "sched.h"
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
#include "task.h"
#include "types.h"
#include "vmm.h"

void schedule();
static void runTask(Task_t *next_task);

extern Task_t *idle_task;
extern Task_t *CurrentTask;
extern TaskList_t TaskList; // 任务链表首节点
extern uint32_t TaskCount;

static RunQueue_t RunQueue;

void initRunQueue();
void enqueueRunQueue(RunQueue_t *rq, Task_t *task);
void dequeueRunQueue(RunQueue_t *rq, Task_t *task);
Task_t *getHeadRunQueue(RunQueue_t *rq);
void tickTask(RunQueue_t *rq, Task_t *task);
void wakeupTask(Task_t *task);
void testRunQueue();

void schedule() {

    // list_ptr_t *nnow, *ntemp;
    // Task_t *nnext = NULL;
    // Task_t *ntask = NULL;

    // bool nintrflag;
    // intr_save(nintrflag);
    // {
    //     nnow = &(CurrentTask->ptr);
    //     ntemp = nnow;
    //     while (1) {
    //         ntemp = listGetNext(ntemp);
    //         if (ntemp == &(TaskList.ptr)) {
    //             continue;
    //         }

    //         ntask = lp2task(ntemp, ptr);
    //         if (TaskCount != 1 && ntask->tid == 0) {
    //             continue;
    //         }
    //         if (ntask->state != TASK_RUNNABLE) {
    //             continue;
    //         } else {
    //             nnext = ntask;
    //             break;
    //         }
    //     }
    //     runTask(nnext);
    // }
    // intr_restore(nintrflag);

    bool intr_flag;
    Task_t *next;
    bool intrflag;
    intr_save(intrflag);
    {
        CurrentTask->need_resched = 0;
        if (CurrentTask->state == TASK_RUNNABLE) {
            //if (CurrentTask != idle_task) {
                enqueueRunQueue(&RunQueue, CurrentTask);
            //}
        }
        if ((next = getHeadRunQueue(&RunQueue)) != NULL) {
            dequeueRunQueue(&RunQueue, next);
        }
        if (next == NULL) {
            next = idle_task;
        }
        if (next != CurrentTask) {
            
            runTask(next);
        }
    }
    intr_restore(intrflag);
    // printk("next%d ",next->tid);
}
void runTask(Task_t *next_task) {
    if (next_task != CurrentTask) {
        Task_t *prev = CurrentTask;
        Task_t *next = next_task;

        bool flag;
        intr_save(flag);
        {
            CurrentTask = next_task;
            load_esp0(next->kstack + KSTACKSIZE);
            lcr3(next->cr3);
            switch_to_s(&(prev->context), &(next->context));
        }
        intr_restore(flag);
    }
}

// 就绪队列
//----------------------------------------------------------------------------------------------

void initRunQueue() {
    RunQueue.max_time_slice = 5;
    RunQueue.task_num = 0;
    initList(&(RunQueue.run_list));
}

void enqueueRunQueue(RunQueue_t *rq, Task_t *task) {
    if (rq == NULL || task == NULL) {
        panic("enqueueRunQueue error!");
    }
    // if (task != idle_task) {
    listAddBefore(&(rq->run_list), &(task->run_queue_ptr));
    //}

    // 如果进程在当前的执行时间片已经用完
    if (task->time_slice == 0 || task->time_slice > rq->max_time_slice) {
        task->time_slice = rq->max_time_slice;
    }
    rq->task_num++;
}

void dequeueRunQueue(RunQueue_t *rq, Task_t *task) {
    if (rq == NULL || task == NULL) {
        panic("dequeueRunQueue error!");
    }
    listDelInit(&(task->run_queue_ptr));
    rq->task_num--;
}

Task_t *getHeadRunQueue(RunQueue_t *rq) {
    list_ptr_t *lp = listGetNext(&(rq->run_list));
    if (lp != &(rq->run_list)) {
        return lp2task(lp, run_queue_ptr);
    }
    return NULL;
}
void tickTask(RunQueue_t *rq, Task_t *task) {
    if (task->time_slice > 0) {
        task->time_slice--;
    }
    if (task->time_slice == 0) {
        task->need_resched = 1;
    }
}
void tickCurrentTask(){
    tickTask(&RunQueue,CurrentTask);
}
//----------------------------------------------------------------------------------------------

void wakeupTask(Task_t *task) {
    if (task->state == TASK_ZOMBIE) {
        panic("try to wake up zombie task!");
    }
    bool flag;
    intr_save(flag);
    {
        if (task->state != TASK_RUNNABLE) {
            task->state = TASK_RUNNABLE;
            task->wait_state = WT_NOTWAIT;

            if (task != CurrentTask) {
                if (task != idle_task) {
                    enqueueRunQueue(&RunQueue, task);
                }
            }
        }
    }
    intr_restore(flag);
}
void testRunQueue() {
    printk("Task in RunQueue:\n");
    list_ptr_t *head = &(RunQueue.run_list);
    list_ptr_t *lp = NULL;
    listForEach(lp, head) {
        Task_t *tk = lp2task(lp, run_queue_ptr);
        printk(" tid:%d\n", tk->tid);
    }
}