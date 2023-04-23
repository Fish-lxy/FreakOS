#include "fat.h"
#include "sem.h"
#include "types.h"

void lockFatFS(FatFs_t *fatfs) { accquireSem(&fatfs->fat_sem); }
void unlockFatFS(FatFs_t *fatfs) { releaseSem(&fatfs->fat_sem); }

void lockFatInode(FatINode_t *fatinode) { accquireSem(&fatinode->sem); }
void unlockFatInode(FatINode_t *fatinode) { releaseSem(&fatinode->sem); }