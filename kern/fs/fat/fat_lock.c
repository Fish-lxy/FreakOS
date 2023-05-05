#include "fat.h"
#include "sem.h"
#include "types.h"

void lockFatFS(FatFs_t *fatfs) { acquireSem(&fatfs->fat_sem); }
void unlockFatFS(FatFs_t *fatfs) { releaseSem(&fatfs->fat_sem); }

void lockFatInode(FatINode_t *fatinode) { acquireSem(&fatinode->sem); }
void unlockFatInode(FatINode_t *fatinode) { releaseSem(&fatinode->sem); }