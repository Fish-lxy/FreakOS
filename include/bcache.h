#ifndef __BCACHE_H
#define __BCACHE_H

#include "dev.h"
#include "types.h"

#define N_BCACHE 4

typedef struct BlockCache_t {
    uint32_t size;
    void *data;

    uint32_t devid;
    Device_t *dev;
    uint32_t secno;

    uint32_t time;
    bool occupied;
    bool modified;
} BlockCache_t;

void initBlockCache();
int diskIO_BCache(Device_t *blockdev, IOrequest_t *in_req);

void sprintAllBCache();
void sprintBCache(uint32_t i);
void testBCache();

#endif