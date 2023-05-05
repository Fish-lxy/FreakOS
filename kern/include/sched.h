#ifndef __SCHED_H
#define __SCHED_H

#include "debug.h"
#include "cpu.h"
#include "kmalloc.h"
#include "types.h"
#include "task.h"
#include "list.h"

#define MAX_TIME_SLICES 1

typedef struct RunQueue_t {
    list_ptr_t run_list;
    unsigned int task_num;
    int max_time_slice;
}RunQueue_t;


void initRunQueue();
void schedule();
void wakeupTask(Task_t* task);
void tickCurrentTask();

void testRunQueue();

#endif