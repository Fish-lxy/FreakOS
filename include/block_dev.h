#ifndef _BLOCK_DEV_H
#define _BLOCK_DEV_H

#include "types.h"

#define MAX_BLOCK_DEV 32

typedef
enum IO_Type_e {
    IO_READ = 0,
    IO_WRITE = 1
} IO_Type_e;

//块设备IO请求
typedef
struct IOrequest_t {
    IO_Type_e io_type;
    uint32_t secno; //申请的块号
    uint32_t nsecs; //申请的块数量
    void* buffer;
    uint32_t bsize; //缓冲区的大小
} IOrequest_t;

//块设备接口
typedef struct BlockDev_ops BlockDev_ops;
//块设备操作
typedef
struct BlockDev_ops {
    int (*init)(void);
    bool (*is_vaild)(void);
    const char* (*get_desc)(void);
    int (*get_nr_block)(void);
    int (*request)(IOrequest_t*);
    int (*ioctl)(int, int);
}BlockDev_ops;

typedef
struct BlockDev_t {
    int32_t id;
    char* name;
    uint32_t block_size;
    void* device;
    BlockDev_ops ops;
} BlockDev_t;



void initBlockdev();
int registerBlockDev(BlockDev_t* dev);


#endif