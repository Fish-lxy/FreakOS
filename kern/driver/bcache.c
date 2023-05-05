#include "bcache.h"
#include "debug.h"
#include "block.h"
#include "ide.h"
#include "kmalloc.h"
#include "sem.h"
#include "string.h"
#include "types.h"
// 块设备缓冲区

static uint32_t SyncTimeGap = 3;

static BlockCache_t *BlockCache;
static Semaphore_t BlockCacheSem;

void initBlockCache() {
    initSem(&BlockCacheSem, 1, "blockcache");
    BlockCache = kmalloc(sizeof(BlockCache_t) * N_BCACHE);
    if (BlockCache == NULL) {
        panic("BlockCache malloc error!");
    }
    for (int i = 0; i < N_BCACHE; i++) {
        BlockCache[i].data = kmalloc(SECTSIZE);
        if (BlockCache[i].data == NULL) {
            panic("BlockCache.data malloc error!");
        }
        BlockCache[i].size = SECTSIZE;
        BlockCache[i].occupied = FALSE;
        BlockCache[i].modified = FALSE;
        BlockCache[i].devid = 0;
        BlockCache[i].dev = NULL;
        BlockCache[i].secno = NULL;
        BlockCache[i].time = 0;
    }
}
static void lockBlockCache() { acquireSem(&BlockCacheSem); }
static void unlockBlockCache() { releaseSem(&BlockCacheSem); }
static void copyIOrequest(IOrequest_t *dst, IOrequest_t *src) {
    dst->bsize = src->bsize;
    dst->buffer = src->buffer;
    dst->io_type = src->io_type;
    dst->nsecs = src->nsecs;
    dst->secno = src->secno;
}
// 直接磁盘IO，一次仅可请求一个磁盘块
static int32_t disk_io_direct_oneblock(BlockDevice_t *blockdev, void *buffer,
                                       uint32_t secno, bool write) {
    if (blockdev == NULL) {
        panic("blockdev is NULL in disk_io!");
    }
    // if (blockdev->type != BlockDevice) {
    //     panic("try to read/write non-BlockDevice in disk_io!");
    // }

    IOrequest_t req;
    req.io_type = (write == TRUE) ? IO_WRITE : IO_READ;
    req.nsecs = 1;
    req.bsize = SECTSIZE;
    req.secno = secno;
    req.buffer = buffer;

    if (blockdev->ops.request(&req) != 0) {
        return 1;
    }

    return 0;
}

// 经过块缓冲区的磁盘IO，LRU算法管理
static int bcache_rw(BlockDevice_t *blockdev, void *buffer, uint32_t secno,
                     bool write) {
    lockBlockCache();
    // 遍历所有缓冲区，找到一个已使用的且其块号和待读写块号相等的缓冲区，则读写该区，跳过后面步骤。
    for (int i = 0; i < N_BCACHE; i++) {
        if (BlockCache[i].occupied && BlockCache[i].dev == blockdev &&
            BlockCache[i].secno == secno) {
            if (write == IO_WRITE) {
                memcpy(BlockCache[i].data, buffer, SECTSIZE);
                BlockCache[i].modified = TRUE;
            } else {
                memcpy(buffer, BlockCache[i].data, SECTSIZE);
            }
            BlockCache[i].time = 0;
            sprintk("bc%d:缓冲区命中 type:%s,", i, write == TRUE ? "写" : "读");
            sprintBCache(i);

            goto update_time;
        }
    }

    // 找到一个未使用缓存，则从磁盘中加载待读写块到缓冲区，读写该块，跳过后面步骤
    uint32_t max = 0;
    for (int i = 0; i < N_BCACHE; i++) {
        if (BlockCache[i].occupied == FALSE) { // 找到一个未使用缓冲区
            BlockCache[i].occupied = TRUE;
            BlockCache[i].secno = secno;
            BlockCache[i].dev = blockdev;
            BlockCache[i].devid = blockdev->id;

            // 从磁盘中加载待读写块到缓冲区
            if (disk_io_direct_oneblock(blockdev, BlockCache[i].data, secno,
                                        IO_READ) != 0) {
                unlockBlockCache();
                return 1;
            }
            if (write == IO_WRITE) {
                memcpy(BlockCache[i].data, buffer, SECTSIZE);
                BlockCache[i].modified = TRUE;
            } else {
                memcpy(buffer, BlockCache[i].data, SECTSIZE);
                BlockCache[i].modified = FALSE;
            }
            BlockCache[i].time = 0;
            sprintk("bc%d:找到空缓冲区,", i);
            sprintBCache(i);

            goto update_time;
        } else if (BlockCache[i].time > BlockCache[max].time)
            max = i;
    }

    // 所有缓冲区都被占用则淘汰时间最大的缓存
    if (BlockCache[max].modified) { // 先将已修改缓冲区写回磁盘
        if (disk_io_direct_oneblock(BlockCache[max].dev, BlockCache[max].data,
                                    BlockCache[max].secno, IO_WRITE) != 0) {
            unlockBlockCache();
            return 1;
        }
        sprintk("bc%d:已写回已修改缓冲区,", max);
        sprintBCache(max);
    }

    // 淘汰时间最大的缓存
    sprintk("bc%d:淘汰缓冲区:", max);
    sprintBCache(max);

    BlockCache[max].time = 0;
    BlockCache[max].occupied = TRUE;
    BlockCache[max].secno = secno;
    BlockCache[max].dev = blockdev;
    BlockCache[max].devid = blockdev->id;
    if (disk_io_direct_oneblock(blockdev, BlockCache[max].data, secno,
                                IO_READ) != 0) {
        unlockBlockCache();
        return 1;
    }
    if (write == IO_WRITE) {
        memcpy(BlockCache[max].data, buffer, SECTSIZE);
        BlockCache[max].modified = TRUE;
    } else {
        memcpy(buffer, BlockCache[max].data, SECTSIZE);
        BlockCache[max].modified = FALSE;
    }

update_time:
    for (int i = 0; i < N_BCACHE; i++) {
        if (BlockCache[i].occupied)
            BlockCache[i].time++;
    }
    unlockBlockCache();
    return 0;
}

// 磁盘IO，经过块缓冲区
int diskIO_BCache(BlockDevice_t *blockdev, IOrequest_t *in_req) {
    if (blockdev == NULL) {
        panic("blockdev is NULL in disk_io!");
    }
    // if (blockdev->type != BlockDevice) {
    //     panic("try to read/write non-BlockDevice in disk_io!");
    // }

    // 分割磁盘请求，一次一个块
    IOrequest_t tempreq;
    tempreq.io_type = in_req->io_type;
    tempreq.nsecs = 1;
    tempreq.bsize = SECTSIZE;
    for (uint32_t i = 0; i < in_req->nsecs; i++) {
        tempreq.secno = in_req->secno + i;
        tempreq.buffer = in_req->buffer + SECTSIZE * i;

        if (bcache_rw(blockdev, tempreq.buffer, tempreq.secno,
                      tempreq.io_type) != 0) {
            return 1;
        }
    }
    return 0;
}
int sync_all_bcache() {
    lockBlockCache();
    sprintk("bc:同步所有缓冲区:\n");
    for (int i = 0; i < N_BCACHE; i++) {
        if (BlockCache[i].occupied && BlockCache[i].modified) {
            sprintBCache(i);
            if (disk_io_direct_oneblock(BlockCache[i].dev, BlockCache[i].data,
                                        BlockCache[i].secno, IO_WRITE) != 0) {
                unlockBlockCache();
                return 1;
            }
        }
    }
    unlockBlockCache();
    return 0;
}

void sync_bcache_task() {
    while (1) {
        sync_all_bcache();
    }

    // CurrentTask->state = TASK_SLEEPING;
}

// 直接读写磁盘
static int32_t disk_io_direct(BlockDevice_t *blockdev, IOrequest_t *req) {
    if (blockdev == NULL) {
        panic("blockdev is NULL in disk_io!");
    }
    // if (blockdev->type != BlockDevice) {
    //     panic("try to read/write non-BlockDevice in disk_io!");
    // }
    // sprintk("读磁盘:secno:%d,nsecs:%d,buffer:0x%08X,bszie:%d\n", req.secno,
    //             req.nsecs, req.buffer, req.bsize);
    if (blockdev->ops.request(req) != 0) {
        return 1;
    }
    return 0;
}

// 分多次直接读写磁盘
static int32_t disk_io_block(BlockDevice_t *blockdev, void *buffer, uint32_t secno,
                             uint32_t nsecs, bool write) {
    if (blockdev == NULL) {
        panic("blockdev is NULL in disk_io!");
    }
    // if (blockdev->type != BlockDevice) {
    //     panic("try to read/write non-BlockDevice in disk_io!");
    // }
    IOrequest_t req;
    req.io_type = (write == TRUE) ? IO_WRITE : IO_READ;
    req.nsecs = 1;
    req.bsize = SECTSIZE;
    for (uint32_t i = 0; i < nsecs; i++) {
        req.secno = secno + i;
        req.buffer = buffer + SECTSIZE * i;

        if (blockdev->ops.request(&req) != 0) {
            return 1;
        }
    }
    return 0;
}

void sprintAllBCache() {
    sprintk("All BCache:\n");
    for (int i = 0; i < N_BCACHE; i++) {
        sprintk(" bc%d devid:%d secno:%d time:%d occupied:%d modified:%d\n", i,
                BlockCache[i].devid, BlockCache[i].secno, BlockCache[i].time,
                BlockCache[i].occupied, BlockCache[i].modified);
    }
}
void sprintBCache(uint32_t i) {
    sprintk(" bc%d devid:%d secno:%d time:%d occupied:%d modified:%d\n", i,
            BlockCache[i].devid, BlockCache[i].secno, BlockCache[i].time,
            BlockCache[i].occupied, BlockCache[i].modified);
}

void testBCache() {
    sprintk("\nBCacheTest:\n");
    sprintAllBCache();

    int n = 2;
    void *buf = kmalloc(SECTSIZE * n);
    BlockDevice_t *dev = getDeviceWithId(0);
    IOrequest_t req;
    req.bsize = SECTSIZE * n;
    req.io_type = IO_READ;
    req.nsecs = n;
    req.secno = 4104;
    req.buffer = buf;
    diskIO_BCache(dev, &req);

    sprintAllBCache();
}