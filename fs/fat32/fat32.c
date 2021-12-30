#include "fat32.h"
#include "mbr.h"
#include "ide.h"
#include "block_dev.h"

#include "types.h"
#include "pmm.h"
#include "debug.h"


extern BlockDev_t main_blockdev;
#define BLOCK_DEV (main_blockdev)

FAT_t* fats;

static uint32_t cluster2sector(FAT_t* fat, uint32_t cluster);
static uint32_t dir2cluster(DirectoryEntry_t* de);
static uint32_t cluster2fatblockn(uint32_t cluster);
static uint32_t cluster2fatblockoffset(uint32_t cluster);
static uint32_t fatblock2sector(FAT_t* fat, uint32_t fatblock);

static uint32_t checkClusterNum(FAT_t* fat, uint32_t cluster);
static void switchNewFatBlock(FAT_t* fat, uint32_t cluster);

static uint32_t readFATblock(FAT_t* fat, uint32_t block);
static void readCluster(FAT_t* fat, uint32_t cluster, void* buffer, uint32_t bsize);

static uint32_t _readfile(FAT_t* fat, DirectoryEntry_t* de, void* buffer, uint32_t bsize);

static void fat_init(FAT_t* fat, uint32_t part_start_sector);
static void checkfat(FAT_t* fat);

void printBPB(FAT_t* fat);
void printFATinfo(FAT_t* fat);
void printFAT(FAT_t* fat);
void listRoot(FAT_t* fat);

// static void read() {

// }
void initFAT() {
    fats = kmalloc(sizeof(FAT_t) * FAT_COUNT);
    for (int i = 0;i < FAT_COUNT;i++) {
        uint32_t part_start_sector = PART_START_SECTOR(i);
        if (part_start_sector == 0)
            continue;
        //printk("%d ", i);
        fat_init(&fats[i], part_start_sector);
        checkfat(&fats[i]);
    }

    // printk("\n");
    // printBPB(&fats[0]);
    // printk("\n");
    // printFATinfo(&fats[0]);
    printk("\n");
    printFAT(&fats[0]);
    printk("\n");
    listRoot(&fats[0]);


    DirectoryEntry_t* de = &(fats[0].root[4]);
    uint32_t up = fats[0].byte_per_cluster;
    uint32_t size_up = ROUNDUP(de->file_size, up);
    uint8_t* buf = kmalloc(size_up);
    printk("up: %d\n", up);
    printk("size_up: %d\n", size_up);
    printk("\n");

    _readfile(&fats[0], de, buf, size_up);

    printk("FAT block: %d\n", fatblock2sector(&fats[0], 1));

    
    // for (int i = 0;i < SECTSIZE * 16;i++) {
    //     if (i == 0) {
    //         printk("%6d/0x%08X: ", i, i);
    //     }
    //     if (i % 16 == 0 && i != 0) {
    //         i += SECTSIZE - 16;
    //         printk("\n");
    //         printk("%6d/0x%08X: ", i, i);
    //     }
    //     printk("%02X ", ((uint8_t*) buf)[i]);
    // }

    for (int i = 0;i < SECTSIZE / 2 ;i++) {
        if (i % 16 == 0) {
            printk("\n");
        }
        printk("%02X ", ((uint8_t*) buf)[i]);
    }
    printk("\n");


}
//簇号转换为扇区号
static uint32_t cluster2sector(FAT_t* fat, uint32_t cluster) {
    return (fat->bpb->nhidden_sectors
        + fat->bpb->nreserved_secter
        + fat->bpb->nFAT * fat->bpb->nsector_per_FAT32
        + (cluster - 2) * fat->bpb->nsecter_per_cluster);
}
//计算目录项的簇号
static uint32_t dir2cluster(DirectoryEntry_t* de) {
    return ((uint32_t) de->first_cluster_high << 16) | de->first_cluster_low;
}

//簇号转换为 FAT 块号
static uint32_t cluster2fatblockn(uint32_t cluster) {
    return cluster / NFAT_ENTRY_PER_BLOCK; // 返回块号
}
//簇号转换为 FAT 块块内偏移
static uint32_t cluster2fatblockoffset(uint32_t cluster) {
    return cluster % NFAT_ENTRY_PER_BLOCK; // 返回块号
}
//FAT 块号转换为扇区号
static uint32_t fatblock2sector(FAT_t* fat, uint32_t fatblock) {
    //每个 FAT 块占用扇区数量
    uint32_t usector_per_fatblock = FAT_BLOCK_SIZE / fat->bpb->nbyte_per_sector;

    return fat->bpb->nhidden_sectors + fat->bpb->nreserved_secter + fatblock * usector_per_fatblock;
}
// FAT 表将会被分成 4KB 大小的块按需读入内存
// 检查簇号是否需要切换 FAT 块
static uint32_t checkClusterNum(FAT_t* fat, uint32_t cluster) {
    uint32_t fatblock = 0, fatoffset = 0;
    //获取簇号对应的FAT块号和块内偏移
    fatblock = cluster2fatblockn(cluster);
    fatoffset = cluster2fatblockoffset(cluster);
    //检查块号对应FAT块是否在内存
    printk("check:\n");
    printk(" cluster: %d\n", cluster);
    printk(" fat block: %d\n", fatblock);
    printk(" fat offset: %d\n", fatoffset);


    if (fatblock != fat->fat1_block_n) {
        printk(" switch!\n");
        switchNewFatBlock(fat, cluster);
    }
    printk(" fat[offset]: %d\n", fat->fat1[fatoffset]);
}
//切换 FAT 块
static void switchNewFatBlock(FAT_t* fat, uint32_t cluster) {
    uint32_t fatblockn = cluster2fatblockn(cluster);
    readFATblock(fat, fatblockn);
}
//读取FAT块到缓冲区
static uint32_t readFATblock(FAT_t* fat, uint32_t block) {
    IOrequest_t req;
    uint32_t b = fatblock2sector(fat, block);

    req.io_type = IO_READ;
    req.secno = b;
    req.nsecs = FAT_BLOCK_SIZE / fat->bpb->nbyte_per_sector;
    req.buffer = fat->fat1;
    req.bsize = FAT_BLOCK_SIZE;
    if (BLOCK_DEV.ops.request(&req) != 0) {
        panic("FAT1 table IO error!");
    }

    fat->fat1_block_n = b;
}
//根据簇号读取分区数据，一次读取一簇
static void readCluster(FAT_t* fat, uint32_t cluster, void* buffer, uint32_t bsize) {
    uint32_t sector = cluster2sector(fat, cluster);
    IOrequest_t req;
    req.io_type = IO_READ;
    req.secno = sector;
    req.nsecs = fat->bpb->nsecter_per_cluster;
    req.buffer = buffer;
    req.bsize = bsize;
    if (BLOCK_DEV.ops.request(&req) != 0) {
        panic("Read Cluster IO error!");
    }

}
// 读文件，通过目录项寻找文件，读到缓冲区，缓冲区大小必须为文件大小向上取整到簇大小，最小为一簇
static uint32_t _readfile(FAT_t* fat, DirectoryEntry_t* de, void* buffer, uint32_t bsize) {
    //printk("Read File:\n");
    uint32_t cluster = dir2cluster(de);

    //printk("file cluster: 0x%08X, %d\n", cluster, cluster);

    checkClusterNum(fat, cluster);

    //printk("start read:\n\n");

    uint32_t bufptr = 0;
    uint32_t offset = 0;

    uint32_t next = 0;
    //文件小于一簇
    if (de->file_size <= fat->byte_per_cluster) {
        readCluster(fat, cluster, buffer, bsize);
        //printk("size on disk: %d\n", fat->byte_per_cluster);
        return;
    }
    else { //文件大于一簇，需要读多次

        next = cluster;
        while (1) { //每次读一簇
            //printk("read!\n");
            readCluster(fat, next, (uint8_t*) buffer + bufptr, fat->byte_per_cluster);
            bufptr += fat->byte_per_cluster;
            offset = cluster2fatblockoffset(next);
            if (fat->fat1[offset] >= CLUSTER_END1 && fat->fat1[offset] <= CLUSTER_END2) {
                //printk("size on disk: %d\n", bufptr);
                return 0;
            }

            next = fat->fat1[offset];
            //printk("next cluster: %d\n", next);
            checkClusterNum(fat, next);
        }
    }
    //printk("bufptr: %d\n", bufptr);

}
static void fat_init(FAT_t* fat, uint32_t part_start_sector) {
    IOrequest_t req;

    //读取BPB
    BIOSParameterBlockSector_t* bpb = (BIOSParameterBlockSector_t*) kmalloc(512);
    req.io_type = IO_READ;
    req.secno = part_start_sector;
    req.nsecs = 1;
    req.buffer = bpb;
    req.bsize = 512;
    if (BLOCK_DEV.ops.request(&req) != 0) {
        panic("BPB IO error!");
    }

    FATInfoSector_t* fatinfo = (FATInfoSector_t*) kmalloc(512);
    req.io_type = IO_READ;
    req.secno = part_start_sector + bpb->fat32_FSInfo_sector;
    req.nsecs = 1;
    req.buffer = fatinfo;
    req.bsize = 512;
    if (BLOCK_DEV.ops.request(&req) != 0) {
        panic("FATinfo IO error!");
    }

    fat->bpb = bpb;
    fat->fatinfo = fatinfo;

    fat->start_sector = part_start_sector;
    fat->bpb_sector = part_start_sector;
    fat->fatinfo_sector = part_start_sector + bpb->fat32_FSInfo_sector;
    fat->data_sector = part_start_sector + bpb->nreserved_secter + bpb->nFAT * bpb->nsector_per_FAT32;
    fat->root_sector = cluster2sector(fat, bpb->fat32_root_cluster);


    fat->byte_per_cluster = bpb->nbyte_per_sector * bpb->nsecter_per_cluster;

    fat->fat_table1_sector = bpb->nreserved_secter + part_start_sector;
    fat->fat_table_size = bpb->nsector_per_FAT32 * bpb->nbyte_per_sector;



    uint32_t* fat1 = (uint32_t*) kmalloc(FAT_BLOCK_SIZE);
    fat->fat1 = fat1;
    readFATblock(fat, 0);


    uint32_t* root_buffer = (uint32_t*) kmalloc(1024);
    req.io_type = IO_READ;
    req.secno = fat->root_sector;
    req.nsecs = 2;
    req.buffer = root_buffer;
    req.bsize = 1024;
    if (BLOCK_DEV.ops.request(&req) != 0) {
        panic("root table IO error!");
    }
    DirectoryEntry_t* root = (DirectoryEntry_t*) root_buffer;

    fat->root = root;




    //printk("fat_table1_sector: %d, 0x%X\n", fat->fat_table1_sector, fat->fat_table1_sector);
    // for (int i = 0;i < SECTSIZE / 2;i++) {
    //     if (i % 16 == 0) {
    //         printk("\n");
    //     }
    //     printk("%02X ", ((uint8_t*) fat1)[i]);
    // }
    // printk("\n");

    printk("\n");
    // for (int i = 0;i < SECTSIZE / 2;i++) {
    //     if (i % 16 == 0) {
    //         printk("\n");
    //     }
    //     printk("%c", sec[i]);
    // }
    // printk("\n");
}
static void checkfat(FAT_t* fat) {
    if (fat->fatinfo->magic1 != FAT_INFO_MAGIC1) {
        panic("FAT partiton error!");
    }
    if (fat->fatinfo->magic2 != FAT_INFO_MAGIC2) {
        panic("FAT partiton error!");
    }
    if (fat->fatinfo->magic3 != FAT_INFO_MAGIC3) {
        panic("FAT partiton error!");
    }

}
void printBPB(FAT_t* fat) {
    BIOSParameterBlockSector_t* bpb = fat->bpb;
    printk("nbyte_per_sector: %d\n", bpb->nbyte_per_sector);
    printk("nsecter_per_cluster: %d\n", bpb->nsecter_per_cluster);
    printk("nreserved_secter: %d\n", bpb->nreserved_secter);
    printk("nFAT: %d\n", bpb->nFAT);
    printk("_nroot_entries: %d\n", bpb->_nroot_entries);
    printk("_nsecters16: %d\n", bpb->_nsecters16);
    printk("media_descriptor: %X\n", bpb->media_descriptor);
    printk("_nsector_per_FAT16: %d\n", bpb->_nsector_per_FAT16);
    printk("nsector_per_track: %d\n", bpb->nsector_per_track);
    printk("nheads: %d\n", bpb->nheads);
    printk("nhidden_sectors: %d\n", bpb->nhidden_sectors);
    printk("nsectors32: %d\n", bpb->nsectors32);
    printk("nsector_per_FAT32: %d\n", bpb->nsector_per_FAT32);
    printk("fat32_flags: %d\n", bpb->fat32_flags);
    printk("fat32_version: %d\n", bpb->fat32_version);
    printk("fat32_root_cluster: %d\n", bpb->fat32_root_cluster);
    printk("fat32_FSInfo_sector: %d\n", bpb->fat32_FSInfo_sector);
    printk("fat32_boot_block_backup: %d\n", bpb->fat32_boot_block_backup);
    //printk("fat32_reserved\n");
    printk("drive_number: %d\n", bpb->drive_number);
    printk("reserved1: %d\n", bpb->reserved1);
    printk("boot_sign: %d\n", bpb->boot_sign);
    printk("volume_number: %d\n", bpb->volume_number);
    //printk("volume_label: %s\n", sec->volume_label);
    //printk("filesystem_type: %s\n", sec->filesystem_type);
}
void printFATinfo(FAT_t* fat) {
    FATInfoSector_t* sec = fat->fatinfo;
    printk("FATinfo:\n");
    printk("magic1: 0x%X\n", sec->magic1);
    printk("magic2: 0x%X\n", sec->magic2);
    printk("nfree_cluster: %d\n", sec->nfree_cluster);
    printk("next_free_cluster: %d\n", sec->next_free_cluster);
    printk("magic3: 0x%X\n", sec->magic3);
}
void printFAT(FAT_t* fat) {
    printk("start_sector: %d\n", fat->start_sector);
    printk("bpb_sector: %d\n", fat->bpb_sector);
    printk("fatinfo_sector: %d\n", fat->fatinfo_sector);
    printk("fat_table1_sector: %d\n", fat->fat_table1_sector);
    printk("data_sector: %d\n", fat->data_sector);
    printk("root_sector: %d\n", fat->root_sector);

    printk("byte_per_cluster: %d\n", fat->byte_per_cluster);
    printk("fat_table_size: %d\n", fat->fat_table_size);
}
void printDirEntry(DirectoryEntry_t* de) {
    printk("0x%02X ", de->attr);
    if (de->attr == 0x0F) {
        printk("l ", de->attr);
    }
    if (de->attr == 0x10) {
        printk("d ", de->attr);
    }
    if (de->attr == 0x00 || de->attr == 0x01 || de->attr == 0x20) {
        printk("f ", de->attr);
    }
    printk("0x%08X ", dir2cluster(de));
    printk("%8d ", de->file_size);

    for (int i = 0;i < 8;i++) {
        if (de->filename[i] != ' ') {
            uint8_t c = de->filename[i];
            if (c >= 'A' && c <= 'Z') {
                c = c + 32;
            }
            printk("%c", c);
        }
        else {
            continue;
        }
    }
    if (de->filename[8] != ' ')
        printk(".");
    for (int i = 8;i < 11;i++) {
        if (de->filename[i] != ' ') {
            uint8_t c = de->filename[i];
            if (c >= 'A' && c <= 'Z') {
                c = c + 32;
            }
            printk("%c", c);
        }
        else {
            continue;
        }
    }

    printk("\n");
}
void listRoot(FAT_t* fat) {
    DirectoryEntry_t* root = fat->root;
    printk("Files of root directory:\n");
    printk("attr:  cluster:   size:    name:\n");
    for (int i = 0;1;i++) {
        if (root[i].filename[0] == 0) {
            break;
        }
        printDirEntry(&root[i]);
    }
    printk("\n");
}