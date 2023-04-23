#ifndef __FAT_H
#define __FAT_H

#include "dev.h"
#include "fat_base.h"
#include "list.h"
#include "partiton.h"
#include "sem.h"
#include "types.h"

#define MAX_FATFS 8

#define INTERNAL_PATH_LEN 512

// 避免头文件循环引用
// struct FATBase_SuperBlock;
// typedef struct _FATFS_SuperBlock FATBase_SuperBlock;
// struct FATBase_Partition;
// typedef struct _PARTITION FATBase_Partition;
// struct FATBase_FILE;
// typedef struct _FIL FATBase_FILE;
// struct FATBase_FILINFO;
// typedef struct _FILINFO FATBase_FILINFO;
// struct FATBase_DIR;
// typedef struct _DIR FATBase_DIR;
struct FileSystem_t;
typedef struct FileSystem_t FileSystem_t;


typedef struct FatFs_t {
    bool active;

    uint32_t devid;
    Device_t *dev;
    uint32_t boot_sector;

    FATBase_SuperBlock *fatbase_sb; // FAT文件系统基础元数据
    FATBase_Partition *fatbase_part; // FAT文件系统与物理设备对应转换表
    uint32_t
        fatbase_part_index; // 在转换表(FATBase_Partition)和Fat_SuperBlock表中的索引

    uint32_t next_id;
    Semaphore_t fat_sem;

    list_ptr_t inode_list; // 此文件系统上的所有已打开的INode

} FatFs_t;

typedef enum FATBaseType_e { FAT_Type_File = 0, FAT_Type_Dir = 1 } FATBaseType_e;

typedef union FATBase_InodeData_u {
    FATBase_FILE file;
    FATBase_DIR dir;
} FATBase_InodeData_u;

typedef struct FatINode_t {
    FatFs_t *fatfs;
    uint32_t id; // 唯一内存索引，同文件系统的相同文件id相同
    char* internal_path;//内部路径 在堆上分配

    FATBaseType_e fatinode_type;
    FATBase_InodeData_u fatbase_data;

    Semaphore_t sem;
    list_ptr_t list_ptr;
} FatINode_t;

#define lp2fatinode(le, member) to_struct((le), FatINode_t, member)
#define fatinode2inode(le, member)                                             \
    to_struct((le), INode_t,                                                   \
              member) // 通过FatINode_t的地址计算出父结构体INode_t的地址
// 示例：
// FatINode_t *finode;
// INode_t* node = fatinode2inode(finode,fatinode);

void setFatFs(FatFs_t *fatfs, MBR_Device_t *md, uint32_t partid, uint32_t fid);
int32_t mountFATFS(FileSystem_t* fs);
int32_t mountFatFs(FatFs_t *fatfs);

void printFatFs(FatFs_t *f);
void testFatInode();

void lockFatFS(FatFs_t *fatfs);
void unlockFatFS(FatFs_t *fatfs);
void lockFatInode(FatINode_t *fatinode);
void unlockFatInode(FatINode_t *fatinode);

#endif