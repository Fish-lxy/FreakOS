#include "cpu.h"
#include "debug.h"
#include "block.h"
#include "ide.h"
#include "kmalloc.h"
#include "sem.h"
#include "string.h"
#include "types.h"

extern IDEdevice IDE_Devices[MAX_IDE];
Semaphore_t IDE_Sem[MAX_IDE];

// IDE0
int ide0_device_init();
int ide0_request_lock(IOrequest_t *req);
bool ide0_device_isvaild();
uint32_t ide0_get_nr_block();
const char *ide0_get_desc();
int ide0_ioctl(int op, int flag);
// IDE1
int ide1_device_init();
int ide1_request_lock(IOrequest_t *req);
bool ide1_device_isvaild();
uint32_t ide1_get_nr_block();
const char *ide1_get_desc();
int ide1_ioctl(int op, int flag);

extern int32_t getNewDevid();
static void lockIDE(uint32_t ideno) { acquireSem(&IDE_Sem[ideno]); }
static void unlockIDE(uint32_t ideno) { releaseSem(&IDE_Sem[ideno]); }
void setIDE_Device(uint32_t ideno, BlockDevice_t *dev) {
    if (dev == NULL) {
        panic("blockdev is NULL in setIDE_Data()!");
    }
    if (ideno == 0) {
        dev->id = getNewDevid();
        dev->name = "IDE 0";
        dev->block_size = SECTSIZE;
        dev->base_device = &IDE_Devices[ideno];
        //dev->type = BlockDevice;
        dev->ops.init = ide0_device_init;
        dev->ops.is_vaild = ide0_device_isvaild;
        dev->ops.request = ide0_request_lock;
        dev->ops.ioctl = ide0_ioctl;
        dev->ops.get_desc = ide0_get_desc;
        dev->ops.get_nr_block = ide0_get_nr_block;
    }
    if (ideno == 1) {
        dev->id = getNewDevid();
        dev->name = "IDE 1";
        dev->block_size = SECTSIZE;
        dev->base_device = &IDE_Devices[ideno];
        //dev->type = BlockDevice;
        dev->ops.init = ide1_device_init;
        dev->ops.is_vaild = ide1_device_isvaild;
        dev->ops.request = ide1_request_lock;
        dev->ops.ioctl = ide1_ioctl;
        dev->ops.get_desc = ide1_get_desc;
        dev->ops.get_nr_block = ide1_get_nr_block;
    }
    if (ideno == 2 || ideno == 3) {
        memset(dev, 0, sizeof(BlockDevice_t));
    }
}

int ide0_device_init() {
    initSem(&IDE_Sem[0], 1,"ide0");
    return _ide_device_init(0);
}
int ide0_request_lock(IOrequest_t *req) {
    int ret = 0;
    lockIDE(0);
    sprintk("%s磁盘:设备号:%d,起始块:%d,块数:%d,缓冲区:0x%08X,缓冲区大小:%dB\n",
            req->io_type == IO_READ ? "读" : "写", 0, req->secno, req->nsecs,
            req->buffer, req->bsize);
    ret = _ide_request(0, req);
    unlockIDE(0);
    return ret;
}
bool ide0_device_isvaild() { return _ide_device_isvaild(0); }
uint32_t ide0_get_nr_block() { return _ide_get_nr_block(0); }
const char *ide0_get_desc() { return _ide_get_desc(0); }
int ide0_ioctl(int op, int flag) { return _ide_ioctl(0, op, flag); }

int ide1_device_init() {
    initSem(&IDE_Sem[1], 1,"ide1");
    return _ide_device_init(1);
}
int ide1_request_lock(IOrequest_t *req) {
    int ret = 0;
    lockIDE(1);
    ret = _ide_request(1, req);
    unlockIDE(1);
    return ret;
}
bool ide1_device_isvaild() { return _ide_device_isvaild(1); }
uint32_t ide1_get_nr_block() { return _ide_get_nr_block(1); }
const char *ide1_get_desc() { return _ide_get_desc(1); }
int ide1_ioctl(int op, int flag) { return _ide_ioctl(1, op, flag); }

// extern BlockDevice_t BlockDevs[MAX_BLOCK_DEV];
// void setIDE_Device(uint32_t ideno) {
//     BlockDevice_t *blockdev = (BlockDevice_t *)kmalloc(sizeof(BlockDevice_t));
//     if (blockdev == NULL) {
//         panic("Can not kmalloc blockdev in setIDE_Data()!");
//     }
//     if (ideno == 0) {
//         blockdev->id = ideno;
//         blockdev->name = "IDE 0";
//         blockdev->block_size = SECTSIZE;
//         blockdev->device = &IDE_Devices[ideno];
//         blockdev->ops.init = ide0_device_init;
//         blockdev->ops.is_vaild = ide0_device_isvaild;
//         blockdev->ops.request = ide0_request_lock;
//         blockdev->ops.ioctl = ide0_ioctl;
//         blockdev->ops.get_desc = ide0_get_desc;
//         blockdev->ops.get_nr_block = ide0_get_nr_block;

//         BlockDevs[ideno] = *blockdev;
//     }
//     if (ideno == 1) {
//         blockdev->id = ideno;
//         blockdev->name = "IDE 1";
//         blockdev->block_size = SECTSIZE;
//         blockdev->device = &IDE_Devices[ideno];
//         blockdev->ops.init = ide1_device_init;
//         blockdev->ops.is_vaild = ide1_device_isvaild;
//         blockdev->ops.request = ide1_request_lock;
//         blockdev->ops.ioctl = ide1_ioctl;
//         blockdev->ops.get_desc = ide1_get_desc;
//         blockdev->ops.get_nr_block = ide1_get_nr_block;

//         BlockDevs[ideno] = *blockdev;
//     }
//     kfree(blockdev, sizeof(BlockDevice_t));
// }
