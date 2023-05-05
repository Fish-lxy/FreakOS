#include "block.h"
#include "debug.h"
#include "ide.h"
#include "kmalloc.h"
#include "types.h"
//物理设备

// BlockDevice_t BlockDevs[MAX_BLOCK_DEV];
uint32_t BlockDevsActiveCount = 0;

BlockDevice_t *BlockDeviceList = NULL;


void printDevice();

void initBlockDevice() {
    printk("Detected block device:\n");

    BlockDeviceList = kmalloc(sizeof(BlockDevice_t));
    if (BlockDeviceList == NULL) {
        panic("BlockDeviceList malloc error!");
    }
    initList(&(BlockDeviceList->list_ptr));

    // 初始化IDE设备
    for (int i = 0; i < MAX_IDE; i++) {
        BlockDevice_t *dev = kmalloc(sizeof(BlockDevice_t));
        if (BlockDeviceList == NULL) {
            panic("block dev malloc error!");
        }
        setIDE_Device(i, dev);

        if (dev->ops.init == NULL || dev->ops.init() != 0) {
            dev->active = FALSE;
            continue;
        }
        if (dev->ops.is_vaild() == TRUE) {
            dev->active = TRUE;
            listAdd(&(BlockDeviceList->list_ptr), &(dev->list_ptr));
            BlockDevsActiveCount++;
        }
    }
    printDevice();

    

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
BlockDevice_t *getDeviceWithId(uint32_t devid) {
    list_ptr_t *listi = NULL;
    BlockDevice_t *dev = NULL;
    listForEach(listi, &(BlockDeviceList->list_ptr)) {
        dev = lp2blockdev(listi, list_ptr);
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
void printDevice(){
    list_ptr_t *listi = NULL;
    listForEach(listi, &(BlockDeviceList->list_ptr)) {
        BlockDevice_t *dev = lp2blockdev(listi, list_ptr);
        if (dev->ops.init == NULL || dev->ops.init() != 0) {
            printk(" IDE %d: does not exist!\n", dev->id);
            continue;
        }
        if (dev->ops.is_vaild() == TRUE) {
            printk(" %d:%s: %dMB, '%s'.\n", dev->id,dev->name,
                   dev->ops.get_nr_block() * dev->block_size / 1024 / 1024,
                   dev->ops.get_desc());
        }
    }
}
int registerBlockDev(BlockDevice_t *dev) {}