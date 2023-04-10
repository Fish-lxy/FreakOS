#include "partiton.h"
#include "dev.h"
#include "debug.h"
#include "kmalloc.h"
#include "list.h"
#include "string.h"
#include "types.h"

extern Device_t* DeviceList;

MBR_Device_t *MBR_DeviceList;
MBR_DiskInfo_t *MBR_Temp;
uint32_t MBR_Count = 0;

static MBR_DiskInfo_t *readMBR_Info(Device_t *blockdev);

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

    list_ptr_t *listi = NULL;
    listForEach(listi, &(DeviceList->list_ptr)) {
        Device_t *dev = lp2dev(listi, list_ptr);
        if(dev->active == TRUE && dev->type == BlockDevice){
            MBR_Temp = readMBR_Info(dev);
            // 检验是否MBR有效标志
            if (MBR_Temp->magic_55 == 0x55 && MBR_Temp->magic_AA == 0xAA) {
                //构造MBR_Device数据
                md_temp = kmalloc(sizeof(MBR_Device_t));
                memset(md_temp, 0, sizeof(MBR_Device_t));
                md_temp->devid = dev->id;
                md_temp->dev = dev;
                memcpy(&(md_temp->mbr_diskinfo), MBR_Temp,
                       sizeof(MBR_DiskInfo_t));
                MBR_Count++;

                listAdd(&(MBR_DeviceList->list_ptr), &(md_temp->list_ptr));
                
            }
        }
    }

    // for (int i = 0; i < MAX_BLOCK_DEV; i++) {
    //     if (BlockDevs[i].active == TRUE) {
    //         MBR_Temp = readMBR_Info(&BlockDevs[i]);
    //         // 检验是否MBR有效标志
    //         if (MBR_Temp->magic_55 == 0x55 && MBR_Temp->magic_AA == 0xAA) {
    //             //构造MBR_Device数据
    //             md_temp = kmalloc(sizeof(MBR_Device_t));
    //             memset(md_temp, 0, sizeof(MBR_Device_t));
    //             md_temp->devid = i;
    //             md_temp->dev = &BlockDevs[i];
    //             memcpy(&(md_temp->mbr_diskinfo), MBR_Temp,
    //                    sizeof(MBR_DiskInfo_t));
    //             MBR_Count++;

    //             listAdd(&(MBR_DeviceList->list_ptr), &(md_temp->list_ptr));
                
    //         }
    //     }
    // }
    printPartitionInfo();
}

static MBR_DiskInfo_t *readMBR_Info(Device_t *blockdev) {
    IOrequest_t req = {IO_READ, 0, 1, MBR_Temp, 512};
    if (blockdev->ops.request(&req) != 0) {
        panic("IO error!");
    }
    return MBR_Temp;
}

int printPartitionInfo() {
    list_ptr_t *lp = &(MBR_DeviceList->list_ptr);
    list_ptr_t *listi = NULL;
    MBR_Device_t *md = NULL;
    PartitionDiskInfo_t *pdi = NULL;
    printk("Partition Info:\n");

    listForEach(listi,lp){
        md = lp2MBR_Device(listi, list_ptr);
        pdi = md->mbr_diskinfo.partinfo;
        for (int i = 0; i < MBR_PARTITION_COUNT; ++i) {
            char *active = NULL;
            if (pdi[i].active == 0x80) {
                active = "0x80 true";
            } else if (pdi[i].active == 0x00) {
                active = "0x00 false";
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
    }
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