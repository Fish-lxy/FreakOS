#include "debug.h"
#include "block.h"
#include "fat.h"
#include "fat_base.h"
#include "inode.h"
#include "kmalloc.h"
#include "list.h"
#include "partiton.h"
#include "sem.h"
#include "string.h"
#include "types.h"
#include "vfs.h"

// FAT兼容层

extern FATBase_SuperBlock *Fat_SuperBlock[]; // fat_base.c
extern FATBase_Partition Fat_Drives[];       // fat_base.c
extern MBR_Device_t *MBR_DeviceList;

uint32_t FatPartitionActiveCount = 0;

void setFatFs(FatFs_t *fatfs, MBR_Device_t *md, uint32_t partid, uint32_t fid);
static FATBase_Partition *setFATBASE_Partition(uint32_t pindex, uint32_t devid,
                                               uint32_t partid);
int32_t mountFatFs(FatFs_t *fatfs);

static void getFatCount();
void printFatFs(FatFs_t *f);

// unused
int32_t mountFatFs(FatFs_t *fatfs) {
    initSem(&fatfs->fat_sem, 1,"fatfs");
    return fatbase_do_mount(fatfs, &(fatfs->fatbase_sb));
}


int32_t fatfs_mount(FileSystem_t *fs) {
    initSem(&(fs->fatfs.fat_sem), 1,"fatfs");
    initList(&(fs->fatfs.inode_list));
    return fatbase_do_mount(&fs->fatfs, &(fs->fatfs.fatbase_sb));
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
    fatfs->dev = md->dev;
    fatfs->boot_sector = md->mbr_diskinfo.partinfo[partid].start_sector;
    fatfs->fatbase_part_index = fid;
    fatfs->next_id = 0;
    if (Fat_SuperBlock[fid] == NULL) {
        Fat_SuperBlock[fid] =
            (FATBase_SuperBlock *)kmalloc(sizeof(FATBase_SuperBlock));
    }
    fatfs->fatbase_sb = Fat_SuperBlock[fid]; // after mounted;
    fatfs->fatbase_part = setFATBASE_Partition(fid, md->devid, partid);
}
// 设置fat驱动所需的FAT_PARTITION表
static FATBase_Partition *setFATBASE_Partition(uint32_t pindex, uint32_t devid,
                                               uint32_t partid) {
    Fat_Drives[pindex].pd = devid;
    Fat_Drives[pindex].pt = partid;
    return &Fat_Drives[pindex];
}

INode_t *get_fatfs_root_inode(FileSystem_t *fs) {
    return fat_find_inode(&fs->fatfs, "/");
}
int32_t fatfs_sync(FileSystem_t *fs){
    return 0;
}

void getFatCount() {
    list_ptr_t *lp = &(MBR_DeviceList->list_ptr);
    list_ptr_t *listi = NULL;
    MBR_Device_t *md = NULL;
    PartitionDiskInfo_t *pdi = NULL;
    listForEach(listi, lp) {
        md = lp2MBR_Device(listi, list_ptr);
        pdi = md->mbr_diskinfo.partinfo;
        for (int i = 0; i < MBR_PARTITION_COUNT; i++) {
            if (pdi[i].partition_type == PartitionType_FAT32) {
                FatPartitionActiveCount++;
            }
        }
    }
}

void printFatFs(FatFs_t *f) {
    printk("active:%d\n", f->active);
    printk("devid:%d\n", f->devid);
    printk("bootsector:%d\n", f->boot_sector);
    printk("fatbase_part_index:%d\n", f->fatbase_part_index);
    printk("superblock:0x%08X\n", (uint32_t)f->fatbase_sb);
    printk("partiton:0x%08X\n", f->fatbase_part);
    printk("pd:%d,pt:%d\n", Fat_Drives[f->fatbase_part_index].pd,
           Fat_Drives[f->fatbase_part_index].pt);
}
