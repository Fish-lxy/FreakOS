#ifndef __IDE_H
#define __IDE_H

#include "types.h"
#include "dev.h"

#define SECTSIZE                512

#define IDE_BSY                 0x80            // IDE驱动器忙 
#define IDE_DRDY                0x40            // IDE驱动器就绪
#define IDE_DF                  0x20            // IDE驱动器错误
#define IDE_ERR                 0x01            // 上一次命令失败

#define IDE_CMD_READ            0x20            // IDE读扇区命令
#define IDE_CMD_WRITE           0x30            // IDE写扇区命令
#define IDE_CMD_IDENTIFY        0xEC            // IDE识别命令

// IDE设备控制端口偏移量
#define ISA_DATA                0x00            // IDE数据端口偏移
#define ISA_ERROR               0x01            // IDE错误端口偏移
#define ISA_PRECOMP             0x01            
#define ISA_CTRL                0x02            // IDE控制端口偏移
#define ISA_SECCNT              0x02
#define ISA_SECTOR              0x03
#define ISA_CYL_LO              0x04
#define ISA_CYL_HI              0x05
#define ISA_SDH                 0x06            // IDE选择端口偏移
#define ISA_COMMAND             0x07            // IDE命令端口偏移
#define ISA_STATUS              0x07            // IDE状态端口偏移

// IDE设备限制值
#define MAX_IDE                 4
#define MAX_NSECS               128             // IDE设备最大操作扇区数
#define MAX_DISK_NSECS          0x10000000      // IDE设备最大扇区号
#define VALID_IDE(ideno)        (((ideno) >= 0) && ((ideno) < MAX_IDE) && (ide_devices[ideno].valid))

// IDE设备身份信息在读取的信息块中的偏移
#define IDE_IDENT_SECTORS       20
#define IDE_IDENT_MODEL         54
#define IDE_IDENT_CAPABILITIES  98
#define IDE_IDENT_CMDSETS       164
#define IDE_IDENT_MAX_LBA       120
#define IDE_IDENT_MAX_LBA_EXT   200

#define IDE_DESC_LEN            40              // IDE设备描述信息尺寸


// IDE设备端口起始端口定义
#define IO_BASE0                0x1F0           // 主IDE设备起始操作端口
#define IO_CTRL0                0x3F4           // 主IDE控制起始控制端口

#define IO_BASE1                0x170
#define IO_CTRL1                0x374

//一般主板有2个IDE通道，每个通道可以接2个IDE硬盘
//每个通道的主从盘的选择通过第6个IO偏移地址寄存器来设置。

typedef
struct IDEchannel_t {
    uint16_t base;        // I/O Base
    uint16_t ctrl;        // Control Base
} IDEchannel_t;

#define IO_BASE(ideno)          (ide_channels[(ideno) >> 1].base)
#define IO_CTRL(ideno)          (ide_channels[(ideno) >> 1].ctrl)

typedef
struct IDEdevice {
    uint8_t valid;        // 0 or 1 (If Device Really Exists)
    uint32_t sets;        // Commend Sets Supported
    uint32_t size;        // Size in Sectors
    uint8_t desc[41];    // Model in String
} IDEdevice;


// void ideInit();

int _ide_device_init(uint16_t ideno);
int _ide_request(uint32_t ideno, IOrequest_t* req);
bool _ide_device_isvaild(uint32_t ideno);
uint32_t _ide_get_nr_block(uint32_t ideno);
const char* _ide_get_desc(uint32_t ideno);
int _ide_ioctl(uint32_t ideno,int op, int flag);



void setIDE_Device(uint32_t ideno);
void setIDE_Device(uint32_t ideno, Device_t* dev);

#endif