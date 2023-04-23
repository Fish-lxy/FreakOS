#ifndef __SEM_H
#define __SEM_H

#include "types.h"
#include "wait.h"

typedef struct Semaphore_t {
    volatile int32_t value;
    Wait_t wait_queue;

} Semaphore_t;

void initSem(Semaphore_t *sem, int32_t value);

void accquireSem(Semaphore_t *sem);
void releaseSem(Semaphore_t *sem);

#endif