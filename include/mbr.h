#ifndef _MBR_H
#define _MBR_H

#include "types.h"
#include "block_dev.h"

#define MBR_SIZE 512
#define MBR_CODE_LEN 446
#define MBR_PARTITION_SIZE 16
#define MBR_PARTITION_COUNT 4

typedef
enum PartitionType_e {
    PartitionType_Unused = 0x00,
    PartitionType_FAT16 = 0x06,
    PartitionType_FAT32 = 0x0B,
    PartitionType_Extend = 0x05,
    PartitionType_NTFS = 0x07,
    PartitionType_LBAextend = 0x0F,
    PartitionType_Linux = 0x83
} PartitionType_e;

typedef
struct PartitionInfo_t {
    uint8_t active;
    uint8_t start_chs[3];
    uint8_t partition_type;
    uint8_t end_chs[3];
    uint32_t start_sector;
    uint32_t nsectors;
} __attribute__((packed)) PartitionInfo_t;

typedef
struct MBR_Info_t {
    uint8_t code[MBR_CODE_LEN];
    PartitionInfo_t partinfo[MBR_PARTITION_COUNT];
    uint8_t magic_55;
    uint8_t magic_AA;
} __attribute__((packed)) MBR_Info_t;

extern MBR_Info_t* mbr;

#define PART_COUNT 4


#define PART_START_SECTOR(x) (mbr->partinfo[(x)].start_sector)
#define PART_TYPE(x) (mbr->partinfo[(x)].partition_type)

void initMBR();
int readMBR_Info(BlockDev_t* blockdev);
int printPartitionInfo();
const char* getPartType(int type);


#endif