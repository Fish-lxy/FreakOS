#include "dev.h"
#include "debug.h"
#include "ide.h"
#include "kmalloc.h"
#include "types.h"

// Device_t BlockDevs[MAX_BLOCK_DEV];
uint32_t BlockDevsActiveCount = 0;

Device_t *DeviceList = NULL;

void initBlockdev() {
    printk("Detected disk device:\n");

    DeviceList = kmalloc(sizeof(Device_t));
    if (DeviceList == NULL) {
        panic("DeviceList malloc error!");
    }
    initList(&(DeviceList->list_ptr));

    for (int i = 0; i < MAX_IDE; i++) {
        Device_t *dev = kmalloc(sizeof(Device_t));
        if (DeviceList == NULL) {
            panic("dev malloc error!");
        }
        setIDE_Device(i, dev);

        if (dev->ops.init == NULL || dev->ops.init() != 0) {
            dev->active = FALSE;
            continue;
        }
        if (dev->ops.is_vaild() == TRUE) {
            dev->active = TRUE;
            listAdd(&(DeviceList->list_ptr), &(dev->list_ptr));
            BlockDevsActiveCount++;
        }
    }

    list_ptr_t *listi = NULL;
    listForEach(listi, &(DeviceList->list_ptr)) {
        Device_t *dev = lp2dev(listi, list_ptr);
        if (dev->ops.init == NULL || dev->ops.init() != 0) {
            printk(" IDE %d: does not exist!\n", dev->id);
            continue;
        }
        if (dev->ops.is_vaild() == TRUE) {
            printk(" IDE %d: %dMB, '%s'.\n", dev->id,
                   dev->ops.get_nr_block() * dev->block_size / 1024 / 1024,
                   dev->ops.get_desc());
        }
    }

    // 仅初始化前两个硬盘为块设备
    // for (int i = 0; i < MAX_IDE; i++) {
    //     setIDE_Device(i);
    //     // 若设备不存在
    //     if (BlockDevs[i].ops.init == NULL) {
    //         BlockDevs[i].active = FALSE;
    //         // printk(" IDE %d: does not exist!\n", i);
    //         continue;
    //     }
    //     if (BlockDevs[i].ops.init() != 0) {
    //         BlockDevs[i].active = FALSE;
    //         // printk(" IDE %d: does not exist!\n", i);
    //         continue;
    //     }
    //     if (BlockDevs[i].ops.is_vaild() == TRUE) {
    //         BlockDevs[i].active = TRUE;
    //         // BlockDevsActiveCount++;
    //         // printk(" IDE %d: %dMB, '%s'.\n", i,
    //         BlockDevs[i].ops.get_nr_block() * BlockDevs[i].block_size / 1024 /
    //             1024,
    //             BlockDevs[i].ops.get_desc();
    //     }
    // }
    // printk("\n");

    
}
//通过设备号搜索对应的设备
Device_t *getDeviceWithDevid(uint32_t devid) {
    list_ptr_t *listi = NULL;
    Device_t *dev = NULL;
    listForEach(listi, &(DeviceList->list_ptr)) {
        dev = lp2dev(listi, list_ptr);
        if (dev != NULL && dev->id == devid) {
            return dev;
        }
    }
    return NULL;
}
int32_t getNewDevid() {
    static int32_t next = -1;
    next++;
    return next;
}
int registerBlockDev(Device_t *dev) {}