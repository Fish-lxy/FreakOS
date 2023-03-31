#ifndef __FAT_FS_H
#define __FAT_FS_H

#include "block_dev.h"
#include "types.h"
// #include "fat.h"

#define MAX_FATFS 8

// 避免头文件循环引用，原型定义在fat.h"
struct FATFS_SuperBlock;
typedef struct _FATFS_SuperBlock FATFS_SuperBlock;
struct FAT_PARTITION;
typedef struct _PARTITION FAT_PARTITION;

typedef struct FatFs_t FatFs_t;
typedef struct FatFs_t {
    bool active;
    
    uint32_t devid;
    uint32_t boot_sector;
    uint32_t fid;

    FATFS_SuperBlock *fatfs_sb; // FAT文件系统元数据
    FAT_PARTITION *fatfs_part;

    int32_t (*mount)(FatFs_t *fatfs);
    int32_t (*sync)(FatFs_t *fatfs);
} FatFs_t;

typedef struct FatFs_Inode_t {

} FatFs_Inode_t;

void detectFatFs();
void testFatFs();
#endif