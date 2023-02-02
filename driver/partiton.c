#include "partiton.h"
#include "block_dev.h"
#include "debug.h"
#include "fat.h"
#include "kmalloc.h"
#include "list.h"
#include "string.h"
#include "types.h"

extern BlockDev_t BlockDevs[MAX_BLOCK_DEV];
extern BlockDev_t MainBlockdev;
extern FAT_PARTITION Drives[8]; // fat.h

MBR_Device_t *MBR_DeviceList;
MBR_DiskInfo_t *MBR_Temp;
uint32_t MBR_Count = 0;

static MBR_DiskInfo_t *readMBR_Info(BlockDev_t *blockdev);

// 从MBR中读取并初始化磁盘分区表信息
void initPartitionTable() {
    MBR_Temp = kmalloc(sizeof(MBR_DiskInfo_t));
    if(MBR_Temp == NULL){
        panic("MBR_Temp malloc error!");
    }
    MBR_DeviceList = kmalloc(sizeof(MBR_Device_t));
    if(MBR_DeviceList == NULL){
        panic("MBR_DeviceList malloc error!");
    }
    MBR_Device_t *md_temp = NULL;

    initList(&(MBR_DeviceList->list_ptr));
    list_ptr_t *next = &(MBR_DeviceList->list_ptr);

    for (int i = 0; i < MAX_BLOCK_DEV; i++) {
        if (BlockDevs[i].active == TRUE) {
            MBR_Temp = readMBR_Info(&BlockDevs[i]);
            // 检验是否MBR有效标志
            if (MBR_Temp->magic_55 == 0x55 && MBR_Temp->magic_AA == 0xAA) {
                //构造MBR_Device数据
                md_temp = kmalloc(sizeof(MBR_Device_t));
                memset(md_temp, 0, sizeof(MBR_Device_t));
                md_temp->devid = i;
                md_temp->dev = &BlockDevs[i];
                memcpy(&(md_temp->mbr_diskinfo), MBR_Temp,
                       sizeof(MBR_DiskInfo_t));

                //插入MBR_Device链表中
                if (i == 0) {
                    MBR_DeviceList->devid = md_temp->devid;
                    MBR_DeviceList->dev = md_temp->dev;
                    memcpy(&(MBR_DeviceList->mbr_diskinfo),
                           &(md_temp->mbr_diskinfo), sizeof(MBR_DiskInfo_t));
                } else {
                    listAdd(&(MBR_DeviceList->list_ptr), &(md_temp->list_ptr));
                }

                MBR_Count++;
            }
            // printk("55:%X AA:%X\n", mbr_temp->magic_55, mbr_temp->magic_AA);
        }
    }
    printPartitionInfo();

    // list_ptr_t *lp = &(MBR_DeviceList->list_ptr);
    // MBR_Device_t *md = lp2MBR_Device(lp, list_ptr);
    // int ii = 0;
    // do {
    //     ii++;
    //     md = lp2MBR_Device(lp, list_ptr);
    //     printk("ii:%d devid:%d dev:0x%08X 55:%X AA:%X\n", ii, md->devid,
    //            (uint32_t)md->dev, md->mbr_diskinfo.magic_55,
    //            md->mbr_diskinfo.magic_AA);
    //     printk("dev:0x%08X\n", &BlockDevs[md->devid]);
    //     lp = listGetNext(lp);
    // } while (lp != &(MBR_DeviceList->list_ptr));

    // printk("mbrcount:%d\n", MBR_Count);

    
    // for (int i = 0; i < MBR_PARTITION_COUNT; i++) {
    //     if (mbr_temp->partinfo[i].partition_type != 0) {
    //         Drives[i].pd = 0;
    //         Drives[i].pd = i;
    //     }
    // }
}

static MBR_DiskInfo_t *readMBR_Info(BlockDev_t *blockdev) {
    IOrequest_t req = {IO_READ, 0, 1, MBR_Temp, 512};
    if (blockdev->ops.request(&req) != 0) {
        panic("IO error!");
    }
    return MBR_Temp;
}

int printPartitionInfo() {
    list_ptr_t *lp = &(MBR_DeviceList->list_ptr);
    MBR_Device_t *md = NULL;
    PartitionDiskInfo_t *pdi = NULL;
    printk("\nPartition Info:\n");

    do {
        md = lp2MBR_Device(lp, list_ptr);
        pdi = md->mbr_diskinfo.partinfo;
        for (int i = 0; i < MBR_PARTITION_COUNT; ++i) {
            char *active = NULL;
            if (pdi[i].active == 0x80) {
                active = "true";
            } else if (pdi[i].active == 0x00) {
                active = "false";
            } else {
                active = "unknown";
            }

            if (pdi[i].partition_type != 0) {
                printk(" Active: %s/0x%02X  ", active, pdi[i].active);
                printk(" Type: %s/0x%02X\n", getPartType(pdi[i].partition_type),
                       pdi[i].partition_type);
                printk(" CHS: %02X%02X%02X", pdi[i].start_chs[0],
                       pdi[i].start_chs[1], pdi[i].start_chs[2]);
                printk("-%02X%02X%02X\n", pdi[i].end_chs[0], pdi[i].end_chs[1],
                       pdi[i].end_chs[2]);
                printk(" Start: %04u  ", pdi[i].start_sector);
                printk("Count: %05u\n", pdi[i].nsectors);
            }
        }
        lp = listGetNext(lp);
    } while (lp != &(MBR_DeviceList->list_ptr));
}
const char *getPartType(int type) {
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