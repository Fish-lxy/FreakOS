#ifndef __DEV_INODE_
#define __DEV_INODE_

#include "block.h"

struct INode_t;
typedef struct INode_t INode_t;
struct INodeOps_t;
typedef struct INodeOps_t INodeOps_t;



typedef enum DeviceType_e { BlockDevice = 0, CharDevice } DeviceType_e;

typedef struct DevINode_t {
    union {
        BlockDevice_t *blkdev;
    };

    DeviceType_e dev_type;

    int32_t blocksize;
    int32_t blocks;
} DevINode_t;


INode_t *create_vdev_inode(INodeOps_t *ops);

void blk_init_vdev();
void stdin_init_vdev();
void stdout_init_vdev();

#endif