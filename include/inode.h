#ifndef __INODE_H
#define __INODE_H

#include "fat.h"

struct FileSystem_t;
typedef struct FileSystem_t FileSystem_t;
struct INode_t;
typedef struct INode_t INode_t;

struct FatInode_t;
typedef struct FatInode_t FatInode_t;
struct FATBase_FILINFO;
typedef struct _FILINFO FATBase_FILINFO;
//----------------------------------------------

typedef enum INodeType_e { FAT_Type, Device_Type } INodeType_e;

typedef struct INodeOps_t {
    int32_t vop_magic;
    int (*vop_open)(INode_t *inode, int flag);
} INodeOps_t;

typedef struct INode_t {
    union {
        FatINode_t fatinode;
    };
    INodeType_e type;

    int32_t ref;
    int32_t open;

    FileSystem_t *fs;
    INodeOps_t *ops;
    
} INode_t;

typedef struct Stat_t {
    int32_t id;
    union {
        // FATBase_FILINFO fat_fileinfo;
    };

} Stat_t;

void inode_ref_inc(INode_t* inode);
void inode_ref_dec(INode_t* inode);
INode_t *inode_alloc(INodeType_e type);
void inode_free(INode_t *inode);
void inode_init(INode_t *inode, INodeOps_t *ops, FileSystem_t *fs);

#endif