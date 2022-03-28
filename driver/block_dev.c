#include "types.h"
#include "block_dev.h"
#include "ide.h"

//块设备结构，前四个为IDE硬盘
BlockDev_t blockdevs[MAX_BLOCK_DEV];

uint32_t blockdevs_index = 0;
BlockDev_t main_blockdev;


void initBlockdev() {
    //初始化前两个硬盘为块设备
    printk("Detected disk device:\n");
    for (int i = 0;i < MAX_IDE - 2;i++) {
        setIde(i);
        
        if(blockdevs[i].ops.init() != 0){//若设备不存在
            continue;
        }
        if(blockdevs[i].ops.is_vaild() == TRUE){
            printk(" IDE %d: %dMB, '%s'.\n"
                , i, blockdevs[i].ops.get_nr_block() * blockdevs[i].block_size / 1024 / 1024, blockdevs[i].ops.get_desc());
        }
        blockdevs_index++;

    }

    main_blockdev = blockdevs[0];
}
int registerBlockDev(BlockDev_t* dev) {
    blockdevs[blockdevs_index] = *dev;
    blockdevs_index++;
}