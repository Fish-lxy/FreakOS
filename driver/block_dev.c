#include "types.h"
#include "block_dev.h"
#include "ide.h"

//块设备结构，前四个为IDE硬盘
BlockDev_t BlockDevs[MAX_BLOCK_DEV];
uint32_t BlockDevsAliveCount = 0;

uint32_t blockdevs_index = 0;
BlockDev_t MainBlockdev;


void initBlockdev() {
    printk("Detected disk device:\n");

    //仅初始化前两个硬盘为块设备
    for (int i = 0;i < MAX_IDE;i++) {
        setIDE_Data(i);
        //若设备不存在
        if(BlockDevs[i].ops.init == NULL){
            BlockDevs[i].active = FALSE;
            printk(" IDE %d: does not exist!\n",i);
            continue;
        }
        if(BlockDevs[i].ops.init() != 0){
            BlockDevs[i].active = FALSE;
            printk(" IDE %d: does not exist!\n",i);
            continue;
        }
        if(BlockDevs[i].ops.is_vaild() == TRUE){
            BlockDevs[i].active = TRUE;
            BlockDevsAliveCount++;
            printk(" IDE %d: %dMB, '%s'.\n"
                , i, BlockDevs[i].ops.get_nr_block() * BlockDevs[i].block_size / 1024 / 1024, BlockDevs[i].ops.get_desc());
        }
        blockdevs_index++;

    }

    MainBlockdev = BlockDevs[0];
}
int registerBlockDev(BlockDev_t* dev) {
    BlockDevs[blockdevs_index] = *dev;
    blockdevs_index++;
}