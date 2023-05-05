#ifndef __VDEV_
#define __VDEV_

#include "list.h"

#define VDEV_OK 0
#define VDEV_PNULL 1
#define VDEV_TYPE_ERR 2
#define VDEV_NAME_CHECK_ERR 3
#define VDEV_INTERNAL_ERR 4


#define VDEV_NAME_LEN 128

struct INode_t;
typedef struct INode_t INode_t;
struct FileSystem_t;
typedef struct FileSystem_t FileSystem_t;

//VFS层设备抽象
typedef struct vDev_t {
    char *name;//堆上分配
    INode_t *inode;
    FileSystem_t *fs;
    bool mountable;
    list_ptr_t ptr;
} vDev_t;

#define lp2vdev(lp, member) to_struct((lp), vDev_t, member)


void init_vDev();

int vdev_add_dev(const char *name, INode_t *inode, bool mountable);
int vdev_add_fs(const char *name, FileSystem_t *fs);

void test_vDev();

#endif