#ifndef _FAT32_H
#define _FAT32_H

#include "types.h"

typedef
struct BIOSParameterBlockSector_t {
    uint8_t jump_instruction[3];
    char oem_id[8];

    //BIOS Parameter Block
    uint16_t nbyte_per_sector;         //每扇区字节数
    uint8_t nsecter_per_cluster;       //每簇扇区数
    uint16_t nreserved_secter;         //保留扇区数
    uint8_t nFAT;                       //FAT个数
    uint16_t _nroot_entries;           //根目录项数，FAT32无效为0
    uint16_t _nsecters16;              //扇区总数 ，用于小于32M
    uint8_t media_descriptor;           //介质描述符
    uint16_t _nsector_per_FAT16;       //每 FAT扇区数，用于小于32M
    uint16_t nsector_per_track;        //每逻辑磁道扇区数
    uint16_t nheads;                   //逻辑磁头数
    uint32_t nhidden_sectors;          //隐藏扇区数
    uint32_t nsectors32;               //扇区总数，用于大于32M
    uint32_t nsector_per_FAT32;        //每 FAT扇区数，用于大于32M
    uint16_t fat32_flags;               //FAT32标志位
    uint16_t fat32_version;             //FAT32的版本
    uint32_t fat32_root_cluster;        //FAT32根目录起始簇
    uint16_t fat32_FSInfo_sector;       //FSInfo结构在保留区的相对扇区号(相对分区起始扇区)
    uint16_t fat32_boot_block_backup;   //Boot 扇区备份扇区号，通常为 6
    uint8_t fat32_reserved[12];         //保留未使用区域
    uint8_t drive_number;               //BIOS物理设备编号，通常软盘为0x00，硬盘为0x80
    uint8_t reserved1;                  //未使用区域，由 Windows NT 使用
    uint8_t boot_sign;                  //标记
    uint32_t volume_number;             //卷标号
    char volume_label[11];              //卷标
    char filesystem_type[8];            //文件系统类型,如 FAT、FAT12、FAT16

    uint8_t code[420];

    uint8_t magic_55;
    uint8_t magic_AA;
} __attribute__((packed)) BIOSParameterBlockSector_t;

typedef
struct FATInfoSector_t {
    uint32_t magic1;            //必须为 0x41615252
    uint8_t reserved1[480];     //保留未使用
    uint32_t magic2;            //必须为 0x61417272
    uint32_t nfree_cluster;    //空闲簇的数量
    uint32_t next_free_cluster; //下一个空闲簇的簇号
    uint8_t reserved2[12];      //保留未使用
    uint32_t magic3;            //必须为 0xAA550000
} __attribute__((packed)) FATInfoSector_t;

#define FAT_INFO_MAGIC1 0x41615252
#define FAT_INFO_MAGIC2 0x61417272
#define FAT_INFO_MAGIC3 0xAA550000

typedef
struct DirectoryEntry_t {
    uint8_t filename[11];           //短文件名，不包括'.'
    uint8_t attr;                   //文件属性
    /*
    文件属性：
    0x00 可读写文件
    0x01 只读文件
    0x02 隐藏文件
    0x04 系统文件
    0x08 卷标
    0x10 子目录
    0x20 归档文件
    0x0F 长文件名属性，由'只读|隐藏|系统|卷标'组成
    */

    uint8_t reserved;               //保留字段，由 Windows NT 使用
    uint8_t creation_time_tenths;   //文件创建时间的10毫秒位
    uint16_t creation_time;         //文件创建时间
    uint16_t creation_date;         //文件创建日期
    uint16_t last_access_date;      //文件最近一次的访问日期

    uint16_t first_cluster_high;    //文件起始簇号高 16 位

    uint16_t last_write_time;       //文件最近一次的修改时间
    uint16_t last_write_date;       //文件最近一次的修改日期

    uint16_t first_cluster_low;     //文件起始簇号低 16 位

    uint32_t file_size;             //文件字节大小

} __attribute__((packed)) DirectoryEntry_t;


typedef
struct FAT_t {
    uint32_t start_sector;      //分区起始扇区
    uint32_t bpb_sector;        //BPB 起始扇区
    uint32_t fatinfo_sector;    //FATinfo 起始扇区
    uint32_t fat_table1_sector; //FAT1 起始扇区 = 保留扇区数 + 分区起始扇区
    uint32_t data_sector;       //数据区起始扇区，且为 2 号簇起点
    uint32_t root_sector;       //根目录起始扇区

    uint32_t byte_per_cluster;  //每簇的大小
    uint32_t fat_table_size;    //FAT1 大小


    BIOSParameterBlockSector_t* bpb;
    FATInfoSector_t* fatinfo;
    uint32_t* fat1;
    uint32_t fat1_block_n;      //当前 FAT 块号
    DirectoryEntry_t* root;
} FAT_t;

#define FAT_COUNT 4
#define FAT32_FAT_ENTRY_SIZE 4

//若 FAT 表大小超过 4KB 则按需分块读入内存
#define FAT_BLOCK_SIZE 4096

//每扇区能存储 FAT 条目的数量
#define NFAT_ENTRY_PER_SECTOR (512 / FAT32_FAT_ENTRY_SIZE) //128
//每 FAT 块能存储 FAT 条目的数量
#define NFAT_ENTRY_PER_BLOCK (FAT_BLOCK_SIZE / FAT32_FAT_ENTRY_SIZE) //1024

#define CLUSTER_END1 0x0FFFFFF8
#define CLUSTER_END2 0x0FFFFFFF






void initFAT();

#endif