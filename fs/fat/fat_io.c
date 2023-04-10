#include "fat_io.h"
#include "types.h"
#include "dev.h"
#include "ide.h"
#include "fat_base.h"
#include "fat_integer.h"

extern Device_t *getDeviceWithDevid(uint32_t devid);//dev.c

extern FATBASE_Partition Fat_Drives[8];

static DRESULT _disk_read(Device_t* blockdev, void* buffer, uint32_t secno, uint32_t nsecs) {
    IOrequest_t req;
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
    Device_t *dev = getDeviceWithDevid(devid);
    return _disk_read(dev, buffer, secno, nsecs);

}
DSTATUS disk_initialize(BYTE dev) {
    return 0;
}
