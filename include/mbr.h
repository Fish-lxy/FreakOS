#ifndef _MBR_H
#define _MBR_H

#include "types.h"
#include "block_dev.h"

#define MBR_SIZE 512
#define MBR_CODE_LEN 446
#define MBR_PARTITION_SIZE 16
#define MBR_PARTITION_COUNT 4

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

#define PART_START_SECTOR(x) (mbr->partinfo[(x)].start_sector)

void initMBR();
int readMBR_Info(BlockDev_t *blockdev);
int printPartitionInfo();
const char* getPartType(int type);


#endif