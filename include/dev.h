#ifndef __DEV_H
#define __DEV_H

#include "types.h"
#include "list.h"

#define MAX_BLOCK_DEV 16

typedef
enum DeviceType_e{
    BlockDevice = 0,
    CharDevice
}DeviceType_e;

typedef
enum IO_Type_e {
    IO_READ = 0,
    IO_WRITE = 1
} IO_Type_e;

//设备IO请求
typedef
struct IOrequest_t {
    IO_Type_e io_type;
    uint32_t secno; //申请的块号
    uint32_t nsecs; //申请的块数量
    void* buffer;
    uint32_t bsize; //缓冲区的大小
} IOrequest_t;

//块设备接口
typedef struct DeviceOps_t DeviceOps_t;
//块设备操作
typedef
struct DeviceOps_t {
    int (*init)(void);
    bool (*is_vaild)(void);
    const char* (*get_desc)(void);//获得设备描述字符串
    int (*get_nr_block)(void);//获得设备总块数(若有的话)
    int (*request)(IOrequest_t*);
    int (*ioctl)(int, int);

    
}DeviceOps_t;

typedef
struct Device_t {
    int32_t id;
    char* name;
    bool active;
    uint32_t block_size;//块大小
    void* base_device;//指向底层驱动
    DeviceType_e type;
    DeviceOps_t ops;

    list_ptr_t list_ptr;
} Device_t;

#define lp2dev(le, member)                 \
    to_struct((le), Device_t, member)



void initDevice();
int registerBlockDev(Device_t* dev);
int32_t getNewDevid();
Device_t* getDeviceWithId(uint32_t devid);


#endif