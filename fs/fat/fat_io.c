#include "fat_io.h"
#include "types.h"
#include "block_dev.h"
#include "ide.h"
#include "fat.h"
#include "fat_integer.h"

IOrequest_t req;

extern BlockDev_t BlockDevs[MAX_BLOCK_DEV];

extern FAT_PARTITION Drives[8];

static DRESULT _disk_read(BlockDev_t* blockdev, void* buffer, uint32_t secno, uint32_t nsecs) {
    req.io_type = IO_READ;
    req.secno = secno;
    req.nsecs = nsecs;
    req.buffer = buffer;
    req.bsize = nsecs * SECTSIZE;
    if (blockdev->ops.request(&req) != 0) {
        return RES_ERROR;
    }
    return RES_OK;
}
DSTATUS disk_status (BYTE dev){
    return 0;
}
DRESULT disk_read(BYTE devid, void* buffer, uint32_t secno, uint32_t nsecs) {
    return _disk_read(&BlockDevs[devid], buffer, secno, nsecs);

}
DSTATUS disk_initialize(BYTE dev) {
    return 0;
}
