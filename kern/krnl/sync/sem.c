#include "sem.h"
#include "intr_sync.h"
#include "sched.h"
#include "types.h"
#include "wait.h"

void initSem(Semaphore_t *sem, int32_t value, char *name) {
    sem->value = value;
    sem->name = name;
    waitlistInit(&(sem->wait_queue));
}
// p操作
static uint32_t down(Semaphore_t *sem, uint32_t wait_state) {
    Wait_t __w;
    Wait_t *wait = &__w;

    bool intr_flag;
    intr_save(intr_flag);
    {
        if (sem->value > 0) {
            sem->value--;
#if DEBUG_SEM_SPRINTK == 1
            sprintk("信号量 %s -1,当前为%d\n",
                    sem->name == NULL ? "NULL" : sem->name, sem->value);
#endif
            intr_restore(intr_flag);
            return 0;
        }
        // 将当前进程加入等待队列
        waitAddCurrent(&(sem->wait_queue), wait, wait_state);
#if DEBUG_SEM_SPRINTK == 1
        sprintk("sem %s:当前进程%d开始睡眠\n",
                sem->name == NULL ? "NULL" : sem->name, CurrentTask->tid);
#endif
    }
    intr_restore(intr_flag);

    
    schedule();// 进行进程调度
    // 当前进程开始睡眠
    // 保持等待，直到其他进程调用up()，唤醒此进程，则此进程从此处继续

    // 当前进程被唤醒，从此处继续
    intr_save(intr_flag);
    {
        // 从等待队列中删去当前进程
        waitDelCurrent(&(sem->wait_queue), wait); 
    }
    intr_restore(intr_flag);

    if (wait->flags != wait_state) {
        return wait->flags;
    }
    return 0;
}

// v操作
static void up(Semaphore_t *sem, uint32_t wait_state) {
    Wait_t *wait;

    bool intr_flag;
    intr_save(intr_flag);
    {
        // 在等待队列中取得队头的进程
        wait = waitlistGetFirst(&(sem->wait_queue));
        if (wait == NULL) {
            sem->value++;
#if DEBUG_SEM_SPRINTK == 1
            sprintk("信号量 %s +1,当前为%d\n",
                    sem->name == NULL ? "NULL" : sem->name, sem->value);
#endif
        } else {
            // 唤醒等待队列队头的进程
            wakeupWait(&(sem->wait_queue), wait, wait_state, TRUE);
#if DEBUG_SEM_SPRINTK == 1
            sprintk("sem:进程%d已唤醒\n", wait->task->tid);
#endif
        }
    }
    intr_restore(intr_flag);
}

void acquireSem(Semaphore_t *sem) {
    uint32_t flag = down(sem, WT_KSEM);
    if (flag != 0) {
        panic("Sem flag is not 0!");
    }
}
void releaseSem(Semaphore_t *sem) { up(sem, WT_KSEM); }