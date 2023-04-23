#include "vfs.h"
#include "debug.h"
#include "fat.h"
#include "inode.h"
#include "kmalloc.h"
#include "partiton.h"
#include "fat_base.h"
#include "sem.h"
#include "types.h"

extern MBR_Device_t *MBR_DeviceList;
void setFatFs(FatFs_t *fatfs, MBR_Device_t *md, uint32_t partid, uint32_t fid);

FileSystem_t FS_List;
FileSystem_t *TempMainFS;
void detectFS();

FileSystem_t *fs_alloc(FileSystemType_t fs_type) {
    FileSystem_t *fs = NULL;
    if ((fs = kmalloc(sizeof(FileSystem_t))) != NULL) {
        fs->type = fs_type;
        return fs;
    }
    return NULL;
}

void fs_add_list(FileSystem_t *fs) {
    listAdd(&FS_List.list_ptr, &fs->list_ptr);
}



void initFS() {
    initList(&FS_List.list_ptr);
    detectFS();

    FileSystem_t *fs = NULL;
    list_ptr_t *fs_list = &FS_List.list_ptr;
    list_ptr_t *listi = NULL;
    listForEach(listi, fs_list) {
        fs = lp2fs(listi, list_ptr); 
        TempMainFS = fs;
        int re = fs->mount(fs);
        printk("fs_return:%d\n",re);
        //printFatFs(&fs->fatfs);
    }
}
// 读取MBR的分区表信息，探测分区，构造FatFs结构
void detectFS() {
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
            case PartitionType_FAT16:
            case PartitionType_FAT32: {
                int partindex = i;

                FileSystem_t *fs = fs_alloc(FileSystemType_FAT);
                fs_add_list(fs);


                setFatFs(&fs->fatfs, md, partindex, fid);
                fs->mount = mountFATFS;

                fid++;
                break;
            }
            default:
                break;
            }
        }
    }
}

void testVFS() {
    printk("\nFile System:\n");
    printk("Files in Partiton 0:\n ");
    int re =  ls("0:/");
    printk("ls return :%d\n",re);
}