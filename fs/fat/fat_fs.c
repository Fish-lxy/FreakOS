#include "dev.h"
#include "debug.h"
#include "fat_base.h"
#include "kmalloc.h"
#include "list.h"
#include "partiton.h"
#include "string.h"
#include "types.h"

// FAT兼容层

extern FATFS_SuperBlock *Fat_SuperBlock[]; // fat.c
extern FATBASE_Partition Fat_Drives[];       // fat.c
extern MBR_Device_t *MBR_DeviceList;

uint32_t FatPartitionActiveCount = 0;
FatFs_t FatFs[MAX_FATFS];

static void setFatFs(FatFs_t *fatfs, MBR_Device_t *md, uint32_t partid, uint32_t fid);
static FATBASE_Partition *setFATBASE_Partition(uint32_t pindex, uint32_t devid,
                                   uint32_t partid);
int32_t mountFatFs(FatFs_t *fatfs);

static void getFatCount();
void printFatFs(FatFs_t *f);

void testFatFs(){
    printk("\nFatFs:\n");
    //testKmalloc();
    detectFatFs();
    printk("Files in Partiton 0:\n ");
    ls("0:/");
}
// 读取MBR的分区表信息，探测FAT分区，构造FatFs结构
void detectFatFs() {
    getFatCount();

    list_ptr_t *mbr_list_head = &(MBR_DeviceList->list_ptr);
    list_ptr_t *listi = NULL;
    MBR_Device_t *md = NULL;
    PartitionDiskInfo_t *pdi = NULL;
    uint32_t fid = 0;

    listForEach(listi, mbr_list_head) {
        md = lp2MBR_Device(listi, list_ptr);
        pdi = md->mbr_diskinfo.partinfo;
        for (int i = 0; i < MBR_PARTITION_COUNT; i++) {
            switch (pdi[i].partition_type) {
            case PartitionType_FAT32: {
                int partindex = i;
                FatFs_t *f = &FatFs[fid];

                setFatFs(f, md, partindex, fid);
                f->mount = mountFatFs;

                
                fid++;
                break;
            }
            default:
                break;
            }
        }
    }

    FatFs_t *f = &FatFs[0];
    f->mount(f);
    // showFAT(f->fatfs_sb);
    //printFatFs(f);
}

int32_t mountFatFs(FatFs_t *fatfs) {
    return fatbase_do_mount(fatfs, &(fatfs->fatbase_sb));
}

// 构造FatFs结构,做mount前的准备工作
// partid分区表中的分区号 fid内部索引
static void setFatFs(FatFs_t *fatfs, MBR_Device_t *md, uint32_t partid, uint32_t fid) {
    if (fatfs == NULL) {
        panic("fatfs is NULL!");
    }
    memset(fatfs, 0, sizeof(FatFs_t));
    fatfs->active = FALSE;
    fatfs->devid = md->devid;
    fatfs->boot_sector = md->mbr_diskinfo.partinfo[partid].start_sector;
    fatfs->fatbase_part_index = fid;
    if (Fat_SuperBlock[fid] == NULL) {
        Fat_SuperBlock[fid] = (FATFS_SuperBlock *)kmalloc(sizeof(FATFS_SuperBlock));
    }
    fatfs->fatbase_sb = Fat_SuperBlock[fid]; // after mounted;
    fatfs->fatbase_part = setFATBASE_Partition(fid, md->devid, partid);
}
// 设置fat驱动所需的FAT_PARTITION表
static FATBASE_Partition *setFATBASE_Partition(uint32_t pindex, uint32_t devid, uint32_t partid) {
    Fat_Drives[pindex].pd = devid;
    Fat_Drives[pindex].pt = partid;
    return &Fat_Drives[pindex];
}



void getFatCount() {
    list_ptr_t *lp = &(MBR_DeviceList->list_ptr);
    list_ptr_t *listi = NULL;
    MBR_Device_t *md = NULL;
    PartitionDiskInfo_t *pdi = NULL;
    listForEach(listi,lp){
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
    printk("pd:%d,pt:%d\n", Fat_Drives[f->fatbase_part_index].pd, Fat_Drives[f->fatbase_part_index].pt);
}


