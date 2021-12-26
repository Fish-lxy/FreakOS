#include "ide.h"
#include "block_dev.h"
#include "debug.h"
#include "types.h"
#include "cpu.h"

extern IDEdevice ide_devices[MAX_IDE];
extern BlockDev_t blockdevs[MAX_BLOCK_DEV];

//IDE0
int ide0_device_init();
int ide0_request(IOrequest_t* req);
bool ide0_device_isvaild();
uint32_t ide0_get_nr_block();
const char* ide0_get_desc();
int ide0_ioctl(int op, int flag);
//IDE1
int ide1_device_init();
int ide1_request(IOrequest_t* req);
bool ide1_device_isvaild();
uint32_t ide1_get_nr_block();
const char* ide1_get_desc();
int ide1_ioctl(int op, int flag);


void setIde(uint32_t ideno) {
    BlockDev_t blockdev;
    if (ideno == 0) {
        blockdev.id = ideno;
        blockdev.name = "IDE 0";
        blockdev.block_size = SECTSIZE;
        blockdev.device = &ide_devices[ideno];
        blockdev.ops.init = ide0_device_init;
        blockdev.ops.is_vaild = ide0_device_isvaild;
        blockdev.ops.request = ide0_request;
        blockdev.ops.ioctl = ide0_ioctl;
        blockdev.ops.get_desc = ide0_get_desc;
        blockdev.ops.get_nr_block = ide0_get_nr_block;

        blockdevs[ideno] = blockdev;
    }
    if (ideno == 1) {
        blockdev.id = ideno;
        blockdev.name = "IDE 1";
        blockdev.block_size = SECTSIZE;
        blockdev.device = &ide_devices[ideno];
        blockdev.ops.init = ide1_device_init;
        blockdev.ops.is_vaild = ide1_device_isvaild;
        blockdev.ops.request = ide1_request;
        blockdev.ops.ioctl = ide1_ioctl;
        blockdev.ops.get_desc = ide1_get_desc;
        blockdev.ops.get_nr_block = ide1_get_nr_block;

        blockdevs[ideno] = blockdev;
    }


}

int ide0_device_init() {
    return _ide_device_init(0);
}
int ide0_request(IOrequest_t* req) {
    return _ide_request(0, req);
}
bool ide0_device_isvaild() {
    return _ide_device_isvaild(0);
}
uint32_t ide0_get_nr_block() {
    return _ide_get_nr_block(0);
}
const char* ide0_get_desc() {
    return _ide_get_desc(0);
}
int ide0_ioctl(int op, int flag) {
    return _ide_ioctl(0, op, flag);
}


int ide1_device_init() {
    return _ide_device_init(1);
}
int ide1_request(IOrequest_t* req) {
    return _ide_request(1, req);
}
bool ide1_device_isvaild() {
    return _ide_device_isvaild(1);
}
uint32_t ide1_get_nr_block() {
    return _ide_get_nr_block(1);
}
const char* ide1_get_desc() {
    return _ide_get_desc(1);
}
int ide1_ioctl(int op, int flag) {
    return _ide_ioctl(1, op, flag);
}

