#include "types.h"
#include "debug.h"
#include "block_dev.h"
#include "mbr.h"
#include "pmm.h"

extern BlockDev_t main_blockdev;
MBR_Info_t* mbr;
void initMBR() {
    mbr = kmalloc(sizeof(MBR_Info_t));
    readMBR_Info(&main_blockdev);

    printPartitionInfo();
}
int readMBR_Info(BlockDev_t* blockdev) {
    IOrequest_t req = { IO_READ, 0, 1, mbr, 512 };
    if (blockdev->ops.request(&req) != 0) {
        panic("IO error!");
    }
}
int printPartitionInfo() {
    printk("\nPartition Info:\n");
    for (int i = 0; i < MBR_PARTITION_COUNT; ++i) {
        char* active = NULL;
        if (mbr->partinfo[i].active == 0x80) {
            active = "true";
        }
        else if (mbr->partinfo[i].active == 0x00) {
            active = "false";
        }
        else {
            active = "unknown";
        }

        if (mbr->partinfo[i].partition_type != 0) {
            printk(" Active: %s/0x%02X  ", active, mbr->partinfo[i].active);
            printk(" Type: %s/0x%02X\n",
                getPartType(mbr->partinfo[i].partition_type), mbr->partinfo[i].partition_type);
            printk(" CHS: %02X%02X%02X",
                mbr->partinfo[i].start_chs[0], mbr->partinfo[i].start_chs[1], mbr->partinfo[i].start_chs[2]);
            printk("-%02X%02X%02X\n",
                mbr->partinfo[i].end_chs[0], mbr->partinfo[i].end_chs[1], mbr->partinfo[i].end_chs[2]);
            printk(" Start: %04u  ", mbr->partinfo[i].start_sector);
            printk("Count: %05u\n", mbr->partinfo[i].nsectors);
        }
    }
}
const char* getPartType(int type) {
    if (type == 0x00) {
        return "Unused";
    }
    if (type == 0x06) {
        return "FAT16";
    }
    if (type == 0x0B) {
        return "FAT32";
    }
    if (type == 0x05) {
        return "Extend";
    }
    if (type == 0x07) {
        return "NTFS";
    }
    if (type == 0x0F) {
        return "LBAextend";
    }
    if (type == 0x83) {
        return "Linux";
    }
    return "Unknown";
}