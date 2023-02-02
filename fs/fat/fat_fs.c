#include "block_dev.h"
#include "debug.h"
#include "fat.h"
#include "kmalloc.h"
#include "list.h"
#include "partiton.h"
#include "string.h"
#include "types.h"

// Fat兼容层

extern MBR_Device_t *MBR_DeviceList;
extern FATFS_SuperBlock *FatFs_sb[]; // fat.c
extern FAT_PARTITION Drives[];       // fat.c

uint32_t FatPartitionActiveCount = 0;
FatFs_t FatFs[MAX_FATFS];

void setFatFs(FatFs_t *fatfs, MBR_Device_t *md, uint32_t partid, uint32_t fid);
static FAT_PARTITION *setPARTITION(uint32_t pindex, uint32_t devid,
                                   uint32_t partid);
int32_t mountFatFs(FatFs_t *fatfs);

static void getFatCount();
void printFatFs(FatFs_t *f);

// 读取MBR的分区表信息
void detectFatFs() {
    getFatCount();

    list_ptr_t *lp = &(MBR_DeviceList->list_ptr);
    MBR_Device_t *md = NULL;
    PartitionDiskInfo_t *pdi = NULL;
    uint32_t fid = 0;
    do {
        md = lp2MBR_Device(lp, list_ptr);
        pdi = md->mbr_diskinfo.partinfo;
        for (int i = 0; i < MBR_PARTITION_COUNT; i++) {
            switch (pdi[i].partition_type) {
            case PartitionType_FAT32:
                int partindex = i;
                FatFs_t *f = &FatFs[fid];

                setFatFs(f, md, partindex, fid);
                f->mount = mountFatFs;

                printFatFs(f);
                fid++;
                break;

            default:
                break;
            }
        }
        lp = listGetNext(lp);
    } while (lp != &(MBR_DeviceList->list_ptr));

    FatFs_t *f = &FatFs[0];
    f->mount(f);
    showFAT(f->fatfs_sb);
    ls("0:/");
}

int32_t mountFatFs(FatFs_t *fatfs) {
    return fat_do_mount(fatfs, &(fatfs->fatfs_sb));
}
// 构造FatFs结构,做mount前的准备工作
// partid分区表中的分区号 fid内部索引
void setFatFs(FatFs_t *fatfs, MBR_Device_t *md, uint32_t partid, uint32_t fid) {
    if (fatfs == NULL) {
        panic("fatfs is NULL!");
    }
    memset(fatfs, 0, sizeof(FatFs_t));
    fatfs->active = FALSE;
    fatfs->devid = md->devid;
    fatfs->boot_sector = md->mbr_diskinfo.partinfo[partid].start_sector;
    fatfs->fid = fid;
    if (FatFs_sb[fid] == NULL) {
        FatFs_sb[fid] = (FATFS_SuperBlock *)kmalloc(sizeof(FATFS_SuperBlock));
    }
    fatfs->fatfs_sb = FatFs_sb[fid]; // after mounted;
    fatfs->fatfs_part = setPARTITION(fid, md->devid, partid);
}
// 设置fat驱动所需的FAT_PARTITION表
FAT_PARTITION *setPARTITION(uint32_t pindex, uint32_t devid, uint32_t partid) {
    Drives[pindex].pd = devid;
    Drives[pindex].pt = partid;
    return &Drives[pindex];
}

void getFatCount() {
    list_ptr_t *lp = &(MBR_DeviceList->list_ptr);
    MBR_Device_t *md = NULL;
    PartitionDiskInfo_t *pdi = NULL;
    do {
        md = lp2MBR_Device(lp, list_ptr);
        pdi = md->mbr_diskinfo.partinfo;
        for (int i = 0; i < MBR_PARTITION_COUNT; i++) {
            if (pdi[i].partition_type == PartitionType_FAT32) {
                int partid = i; //

                FatPartitionActiveCount++;
            }
        }
        lp = listGetNext(lp);
    } while (lp != &(MBR_DeviceList->list_ptr));
}
void printFatFs(FatFs_t *f) {
    printk("active:%d\n", f->active);
    printk("devid:%d\n", f->devid);
    printk("bootsector:%d\n", f->boot_sector);
    printk("fid:%d\n", f->fid);
    printk("superblock:0x%08X\n", (uint32_t)f->fatfs_sb);
    printk("partiton:0x%08X\n", f->fatfs_part);
    printk("pd:%d,pt:%d\n", Drives[f->fid].pd, Drives[f->fid].pt);
}

int32_t initFatFs(FatFs_t *fatfs, uint32_t devid, uint32_t fid,
                  uint32_t bootsect) {

    fatfs->devid = devid;
    fatfs->fid = fid;
    fatfs->boot_sector = bootsect;
}
