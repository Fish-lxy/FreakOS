#ifndef __INODE_H
#define __INODE_H

#include "dev_inode.h"
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

#define INODE_OPS_MAGIC 0x12345678

#define INODE_TYPE_MASK 0xFF
#define INODE_TYPE_REGULAR 0x1
#define INODE_TYPE_DIR 0x2
#define INODE_TYPE_CHR 0x4
#define INODE_TYPE_BLOCK 0x8

#define INODE_OK 0
#define INODE_PNULL 1
#define INODE_NOT_FILE 2
#define INDOE_NOT_DIR 3
#define INODE_DENIED 4
#define INODE_EXIST 5
#define INODE_NOT_EXIST 6
#define INODE_DISK_ERR 7
#define INODE_INTERNAL_ERR 8
#define INODE_NOT_DEV 9
#define INODE_UNKNOWN 255

#define INODE_CREATE_MASK 0xFF
#define INODE_CREATE_NEW 0x0
#define INODE_CREATE_EXIST 0x1

typedef enum INodeType_e { FAT_Type, Dev_Type } INodeType_e;

typedef struct INodeOps_t {
    int32_t inode_magic;
    int (*iop_lookup)(INode_t *inode, char *relative_path, INode_t **inode_out);
    int (*iop_open)(INode_t *inode, int flag);
    int (*iop_read)(INode_t *inode, char *buffer, uint32_t len);
    int (*iop_write)(INode_t *inode, char *buffer, uint32_t len);
    int (*iop_lseek)(INode_t *inode, int32_t off);
    int (*iop_truncate)(INode_t *inode, uint32_t len);
    int (*iop_create)(INode_t *inode, char *name, uint32_t mode);

    int (*iop_readdir)(INode_t *inode, INode_t **inode_out);
    int (*iop_mkdir)(INode_t *inode, char *dir_name);

    int (*iop_dev_io)(INode_t *inode, uint32_t secno, uint32_t nsec,
                      char *buffer, uint32_t blen, bool write);
    int (*iop_dev_ioctl)(INode_t *inode, int ctrl);

    int (*iop_release)(INode_t *inode);
    int (*iop_delete)(INode_t *inode, char *name);
    int (*iop_rename)(INode_t *inode, char *new);
    int (*iop_stat)(INode_t *inode, Stat_t *stat);
    int (*iop_sync)(INode_t *inode);
    int (*iop_get_type)(INode_t *inode, int32_t *type_out);
} INodeOps_t;

typedef struct INode_t {
    union {
        FatINode_t fatinode;
        DevINode_t devinode;
    };
    INodeType_e inode_type; // 类型

    int32_t ref;
    int32_t open;

    FileSystem_t *fs;
    INodeOps_t *ops;

} INode_t;

typedef struct Stat_t {
    uint32_t attr;
    uint32_t size;

} Stat_t;

void inode_ref_inc(INode_t *inode);
void inode_ref_dec(INode_t *inode);
INode_t *inode_alloc(INodeType_e type);
void inode_free(INode_t *inode);
void inode_init(INode_t *inode, INodeOps_t *ops, FileSystem_t *fs);

#endif