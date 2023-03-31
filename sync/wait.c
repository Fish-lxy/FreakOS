#include "list.h"
#include "types.h"

#include "task.h"
#include "wait.h"

// 唤醒与wait关联的进程
void wakeupWait(Wait_t *head, Wait_t *wait, uint32_t wakeup_flags, bool del) {
    if (del != FALSE) {
        waitlistDel(head, wait);
    }
    wait->flags = wakeup_flags;
    // wakeTask(wait->proc);
}
// 唤醒等待队列上挂着的第一个wait所关联的进程
void wakeupFirst(Wait_t *head, uint32_t wakeup_flags, bool del) {
    Wait_t *wait = waitlistGetFirst(head);
    if (wait != NULL) {
        wakeupWait(head, wait, wakeup_flags, del);
    }
}
// 唤醒等待队列上所有的等待的进程
void wakeupAll(Wait_t *head, uint32_t wakeup_flags, bool del) {
    Wait_t *wait = waitlistGetFirst(head);
    if (wait != NULL) {
        if (del != FALSE) {
            do {
                wakeupWait(head, wait, wakeup_flags, TRUE);
            } while ((wait = waitlistGetFirst(head)) != NULL);
        } else {
            do {
                wakeupWait(head, wait, wakeup_flags, FALSE);
            } while ((wait = waitlistGetNext(head, wait)) != NULL);
        }
    }
}
// 让wait与进程关联，且让当前进程关联的wait进入等待队列list，当前进程睡眠
void waitAddCurrent(Wait_t *head, Wait_t *wait, uint32_t wait_state) {
    assert(CurrentTask != NULL, "CurrentTask is NULL!");
    waitInit(wait, CurrentTask);
    CurrentTask->state = TASK_SLEEPING;
    CurrentTask->wait_state = wait_state;
    waitlistAdd(head, wait);
}
// 把与当前进程关联的wait从等待队列queue中删除
void waitDelCurrent(Wait_t *head, Wait_t *wait) {
    if (waitlistIsInList(wait)) {
        waitlistDel(head, wait);
    }
}

// 初始化Wait_t结构
void waitInit(Wait_t *wait, Task_t *task) {
    wait->task = task;
    wait->flags = WT_INTERRUPTED;
    initList(&(wait->ptr));
}
// 初始化等待队列
void waitlistInit(Wait_t *head) { initList(&(head->ptr)); }
// 尾插法插入等待队列
void waitlistAdd(Wait_t *head, Wait_t *elem) {
    listAddTail(&(head->ptr), &(elem->ptr));
}
// 删除等待队列的元素
void waitlistDel(Wait_t *head, Wait_t *elem) { listDelInit(&(elem->ptr)); }
// 返回等待队列是否为空
bool waitlistIsEmpty(Wait_t *head) { return listIsEmpty(&(head->ptr)); }
// 检查elem是否在等待队列中
bool waitlistIsInList(Wait_t *elem) { return !listIsEmpty(&(elem->ptr)); }
// 获得elem的后继元素指针
Wait_t *waitlistGetNext(Wait_t *head, Wait_t *elem) {
    list_ptr_t *lp = listGetNext(&(elem->ptr));
    if (lp != &(head->ptr)) {
        return lp2wait(lp, ptr);
    }
    return NULL;
}
// 获得elem的前序元素指针
Wait_t *waitlistGetPrev(Wait_t *head, Wait_t *elem) {
    list_ptr_t *lp = listGetPrev(&(elem->ptr));
    if (lp != &(head->ptr)) {
        return lp2wait(lp, ptr);
    }
    return NULL;
}
// 获得等待队列的首个元素
Wait_t *waitlistGetFirst(Wait_t *head) {
    list_ptr_t *lp = listGetNext(&(head->ptr));
    if (lp != &(head->ptr)) {
        return lp2wait(lp, ptr);
    }
    return NULL;
}
// 获得等待队列的尾元素
Wait_t *waitlistGetLast(Wait_t *head) {
    list_ptr_t *lp = listGetPrev(&(head->ptr));
    if (lp != &(head->ptr)) {
        return lp2wait(lp, ptr);
    }
    return NULL;
}