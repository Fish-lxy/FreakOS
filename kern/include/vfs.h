#ifndef _VFS_H
#define _VFS_H

#include "fat.h"
#include "types.h"

// typedef enum FS_Type_t {
//     fat_fs,
//     freak_fs,
// } FS_Type_t;

// typedef struct FS_t {
//     uint32_t id;
//     union {
//         FatFs_t __fat_fs;
//     } fs_info;
//     FS_Type_t type;
//     PartitionDiskInfo_t *mbr_info;
//     BlockDevice_t *dev;
//     uint32_t start_sector;

// } FS_t;

struct FatFs_t;
typedef struct FatFs_t FatFs_t;
struct INode_t;
typedef struct INode_t INode_t;
struct INodeOps_t;
typedef struct INodeOps_t INodeOps_t;

//-------------------------------------------------------

typedef enum FileSystemType_t { FileSystemType_FAT } FileSystemType_t;

typedef struct FileSystem_t {
    union {
        FatFs_t fatfs;
    };
    FileSystemType_t type;

    int32_t (*mount)(FileSystem_t *fs);
    int32_t (*sync)(FileSystem_t *fs);
    INode_t *(*get_root_inode)(FileSystem_t *fs);

    list_ptr_t list_ptr;
} FileSystem_t;

#define lp2fs(le, member) to_struct((le), FileSystem_t, member)

// 示例：
// FatFs_t *i;
// FileSystem_t* fs = fatfs2fs(i,fatfs);
#define fatfs2fs(le, member) to_struct((le), FileSystem_t, member)

void initFS();
void detectFS();

void fs_init_vdev();

void testVFS();

#endif