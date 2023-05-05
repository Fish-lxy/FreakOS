#ifndef __SEM_H
#define __SEM_H

#include "types.h"
#include "wait.h"

typedef struct Semaphore_t {
    volatile int32_t value;
    Wait_t wait_queue;

    char* name;

} Semaphore_t;


void initSem(Semaphore_t *sem, int32_t value , char* name);

void acquireSem(Semaphore_t *sem);
void releaseSem(Semaphore_t *sem);

#endif