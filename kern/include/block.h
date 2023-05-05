#ifndef __DEV_H
#define __DEV_H

#include "list.h"
#include "types.h"

#define MAX_BLOCK_DEV 16


typedef enum IO_Type_e { IO_READ = 0, IO_WRITE = 1 } IO_Type_e;

// 设备IO请求
typedef struct IOrequest_t {
    IO_Type_e io_type;
    uint32_t secno; // 申请的块号
    uint32_t nsecs; // 申请的块数量
    void *buffer;
    uint32_t bsize; // 缓冲区的大小
} IOrequest_t;

typedef struct BlockDeviceOps_t BlockDeviceOps_t;
// 设备操作
typedef struct BlockDeviceOps_t {
    int (*init)(void);
    bool (*is_vaild)(void);
    const char *(*get_desc)(void); // 获得设备描述字符串
    int (*get_nr_block)(void);     // 获得设备总块数(若有的话)
    int (*request)(IOrequest_t *);
    int (*ioctl)(int, int);

} BlockDeviceOps_t;

// 物理设备
typedef struct BlockDevice_t {
    int32_t id;
    char *name;
    bool active;
    uint32_t block_size; // 块大小
    void *base_device;   // 指向底层驱动
    BlockDeviceOps_t ops;

    list_ptr_t list_ptr;
} BlockDevice_t;

#define lp2blockdev(le, member) to_struct((le), BlockDevice_t, member)

void initBlockDevice();
int registerBlockDev(BlockDevice_t *dev);
int32_t getNewDevid();
BlockDevice_t *getDeviceWithId(uint32_t devid);

#endif