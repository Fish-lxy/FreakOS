#ifndef __FAT_FS_H
#define __FAT_FS_H

#include "dev.h"
#include "types.h"
// #include "fat.h"

#define MAX_FATFS 8

// 避免头文件循环引用，原型定义在fat.h"
struct FATFS_SuperBlock;
typedef struct _FATFS_SuperBlock FATFS_SuperBlock;
struct FATBASE_Partition;
typedef struct _PARTITION FATBASE_Partition;

typedef struct FatFs_t FatFs_t;
typedef struct FatFs_t {
    bool active;
    
    uint32_t devid;
    uint32_t boot_sector;
    
    FATFS_SuperBlock *fatbase_sb; // FAT文件系统基础元数据
    FATBASE_Partition *fatbase_part;//FAT文件系统与物理设备对应转换表
    uint32_t fatbase_part_index;//在转换表中的索引

    int32_t (*mount)(FatFs_t *fatfs);
    int32_t (*sync)(FatFs_t *fatfs);
} FatFs_t;

typedef struct FatFs_Inode_t {

} FatFs_Inode_t;

void detectFatFs();
void testFatFs();
#endif