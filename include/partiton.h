#ifndef __PARTITION_H
#define __PARTITION_H

#include "dev.h"
// #include "fat.h"
#include "list.h"
#include "types.h"

#define MBR_SIZE 512
#define MBR_CODE_LEN 446
#define MBR_PARTITION_SIZE 16
#define MBR_PARTITION_COUNT 4

typedef enum PartitionType_e {
    PartitionType_Unused = 0x00,
    PartitionType_FAT16 = 0x06,
    PartitionType_FAT32 = 0x0B,
    PartitionType_Extend = 0x05,
    PartitionType_NTFS = 0x07,
    PartitionType_LBAextend = 0x0F,
    PartitionType_Linux = 0x83
} PartitionType_e;

// 磁盘上的分区结构信息
typedef struct PartitionDiskInfo_t {
    uint8_t active; // 分区状态：00-->非活动分区；80-->活动分区；其它数值没有意义
    uint8_t start_chs[3];   // 分区起始CHS
    uint8_t partition_type; // 文件系统标志位
    uint8_t end_chs[3];     // 分区结束CHS
    uint32_t start_sector;  // 分区起始相对扇区号
    uint32_t nsectors;      // 分区总的扇区数
} __attribute__((packed)) PartitionDiskInfo_t;

// 磁盘上的MBR结构信息
typedef struct MBR_DiskInfo_t {
    uint8_t code[MBR_CODE_LEN];
    PartitionDiskInfo_t partinfo[MBR_PARTITION_COUNT]; // 分区表
    uint8_t magic_55;
    uint8_t magic_AA;
} __attribute__((packed)) MBR_DiskInfo_t;

typedef struct MBR_Device_t {
    uint32_t devid;
    Device_t *dev;
    MBR_DiskInfo_t mbr_diskinfo;
    list_ptr_t list_ptr;
} MBR_Device_t;

#define lp2MBR_Device(le, member) to_struct((le), MBR_Device_t, member)

extern MBR_DiskInfo_t *MBR_Temp;

#define PART_COUNT 4

#define PART_START_SECTOR(x) (mbr->partinfo[(x)].start_sector)
#define PART_TYPE(x) (mbr->partinfo[(x)].partition_type)



void initPartitionTable();

int printPartitionInfo();
const char *getPartType(int type);

#endif