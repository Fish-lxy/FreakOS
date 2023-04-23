#include "fat_io.h"
#include "bcache.h"
#include "debug.h"
#include "dev.h"
#include "fat_base.h"
#include "fat_integer.h"
#include "ide.h"
#include "types.h"

extern Device_t *getDeviceWithId(uint32_t devid); // dev.c

// 读磁盘，分块多次读取
static DRESULT _disk_read(Device_t *blockdev, void *buffer, uint32_t secno,
                           uint32_t nsecs) {
    IOrequest_t req;
    req.io_type = IO_READ;
    req.nsecs = 1;
    req.bsize = SECTSIZE;
    for (uint32_t i = 0; i < nsecs; i++) {
        req.secno = secno + i;
        req.buffer = buffer + SECTSIZE * i;

        // if (blockdev->ops.request(&req) != 0) {
        //     return RES_ERROR;
        // }
        if (diskIO_BCache(blockdev, &req) != 0) {
            return RES_ERROR;
        }
    }
    return RES_OK;
}
static DRESULT _disk_write(Device_t *blockdev, void *buffer, uint32_t secno,
                           uint32_t nsecs) {
    IOrequest_t req;
    req.io_type = IO_WRITE;
    req.nsecs = 1;
    req.bsize = SECTSIZE;
    for (uint32_t i = 0; i < nsecs; i++) {
        req.secno = secno + i;
        req.buffer = buffer + SECTSIZE * i;

        // if (blockdev->ops.request(&req) != 0) {
        //     return RES_ERROR;
        // }
        if (diskIO_BCache(blockdev, &req) != 0) {
            return RES_ERROR;
        }
    }
    return RES_OK;
}

DRESULT fatbase_disk_read(BYTE devid, void *buffer, uint32_t secno,
                          uint32_t nsecs) {
    Device_t *dev = getDeviceWithId(devid);
    return _disk_read(dev, buffer, secno, nsecs);
}

DRESULT fatbase_disk_write(BYTE devid, void *buffer, uint32_t secno,
                          uint32_t nsecs) {
    Device_t *dev = getDeviceWithId(devid);
    return _disk_write(dev, buffer, secno, nsecs);
}


DSTATUS fatbase_disk_status(BYTE devid) { return 0; }
DSTATUS fatbase_disk_initialize(BYTE devid) { return 0; }
DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
){
    return 0;
}

DWORD get_fattime(void){
    return 0;
}
