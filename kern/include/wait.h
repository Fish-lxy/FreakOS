#ifndef __WAIT_H
#define __WAIT_H

#include "list.h"
//#include "task.h"
#include "types.h"

struct Task_t;
typedef struct Task_t  Task_t;


// 等待队列
typedef struct Wait_t {
    Task_t *task;
    uint32_t flags;
    list_ptr_t ptr;
} Wait_t;

#define lp2wait(lp, member) to_struct((lp), Wait_t, member)

void waitInit(Wait_t *wait, Task_t *task);    // 初始化Wait_t结构
void waitlistInit(Wait_t *head);              // 初始化等待队列
void waitlistAdd(Wait_t *head, Wait_t *elem); // 尾插法插入等待队列
void waitlistDel(Wait_t *head, Wait_t *elem); // 删除等待队列的元素
bool waitlistIsEmpty(Wait_t *head);           // 返回等待队列是否为空
bool waitlistIsInList(Wait_t *elem); // 检查elem是否在等待队列中
Wait_t *waitlistGetNext(Wait_t *head, Wait_t *elem); // 获得elem的后继元素指针
Wait_t *waitlistGetPrev(Wait_t *head, Wait_t *elem); // 获得elem的前序元素指针
Wait_t *waitlistGetFirst(Wait_t *head); // 获得等待队列的首个元素
Wait_t *waitlistGetLast(Wait_t *head);  // 获得等待队列的尾元素

// 唤醒与wait关联的进程
void wakeupWait(Wait_t *head, Wait_t *wait, uint32_t wakeup_flags, bool del);
// 唤醒等待队列上挂着的第一个wait所关联的进程
void wakeupFirst(Wait_t *head, uint32_t wakeup_flags, bool del);
// 唤醒等待队列上所有的等待的进程
void wakeupAll(Wait_t *head, uint32_t wakeup_flags, bool del);
// 让wait与进程关联，且让当前进程关联的wait进入等待队列list，当前进程睡眠
void waitAddCurrent(Wait_t *head, Wait_t *wait, uint32_t wait_state);
// 把与当前进程关联的wait从等待队列queue中删除
void waitDelCurrent(Wait_t *head, Wait_t *wait);

#endif