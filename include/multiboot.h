#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include "types.h"

// GNU Multiboot 标准
// 来自 https://www.gnu.org/software/grub/manual/multiboot/multiboot.html

// 引导程序引导32位内核时，机器状态必须如下：
// 1. CS 指向基地址为 0x00000000，限长为4G – 1的代码段描述符。
// 2. DS，SS，ES，FS 和 GS 指向基地址为0x00000000，限长为4G – 1的数据段描述符。
// 3. A20 地址线必须打开。
// 4. CR0 必须清除第31位（PG）。必须设置第0位（PE）。其他位未定义.
// 5. EFLAGS 必须清除第17位（VM）。必须清除第 9 位（IF）。其他位未定义.
// 6. 页机制被禁止。
// 7. 中断被禁止。
// 8. EAX = 0x2BADB002
// 9. EBX 需保存 Multiboot 信息结构的32位地址
// 9. 系统信息和启动信息块的线性地址保存在 EBX中（相当于一个指针）。

//Multiboot 信息结构，来自 Multiboot 标准
typedef
struct MultiBoot_t { 
    // 标志位，表示本结构中其他字段是否存在和有效。
    // 还可用作版本指示，以便在将来进行扩展而不破坏任何内容。
    uint32_t flags;			
    
    // 从 BIOS 获知的可用内存
    // mem_lower和mem_upper分别表示了低端和高端内存的大小，单位是KB。
    // 低端内存的首地址是0，高端内存的首地址是1M。
    // 低端内存的最大可能值是640K。
    // 高端内存的最大可能值是最大值减去1M。但并不保证是这个值。
    // 如果在 flags 字段中设置了第0位为1，则 mem_* 字段有效。
    uint32_t mem_lower;
    uint32_t mem_upper;
    
    // 指出引导程序从哪个BIOS磁盘设备载入的OS映像
    // 如果在 flags 字段中设置了第1位为1，则 boot_device 字段有效。
    uint32_t boot_device;		
    
    uint32_t cmdline;		// 内核命令行物理地址
    uint32_t mods_count;		// boot 模块列表
    uint32_t mods_addr;

    /**
     * ELF 格式内核映像的section头表、每项的大小、一共有几项以及作为名字索引的字符串表。
     * 它们对应于可执行可连接格式（ELF）的program头中的shdr_* 项（shdr_num等）。
     * 所有的section都会被载入，ELF section头的物理地址域指向所有的section在内存中的位置。
     */
    uint32_t num;
    uint32_t size;
    uint32_t addr;
    uint32_t shndx;

    /**
     * 以下两项指出保存由BIOS提供的内存分布的缓冲区的地址和长度
     * mmap_addr是缓冲区的地址，mmap_length是缓冲区的总大小
     * 缓冲区由一个或者多个下面的大小/结构对 mmap_entry_t（size实际上是用来跳过下一个对的）组成
     */
    uint32_t mmap_length;
    uint32_t mmap_addr;

    uint32_t drives_length; 	// 指出第一个驱动器结构的物理地址	
    uint32_t drives_addr; 		// 指出第一个驱动器这个结构的大小
    uint32_t config_table; 		// ROM 配置表
    uint32_t boot_loader_name; 	// boot loader 的名字
    uint32_t apm_table; 	    	// APM 表
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint32_t vbe_mode;
    uint32_t vbe_interface_seg;
    uint32_t vbe_interface_off;
    uint32_t vbe_interface_len;
} __attribute__((packed)) MultiBoot_t;

/**
 * size是相关结构的大小，单位是字节，它可能大于最小值20
 * base_addr_low是启动地址的低32位，base_addr_high是高32位，启动地址总共有64位
 * length_low是内存区域大小的低32位，length_high是内存区域大小的高32位，总共是64位
 * type是相应地址区间的类型，1代表可用RAM，所有其它的值代表保留区域
 */
typedef
struct mmapEntry_t {
    uint32_t size; 		// size 不含 size 自身的大小
    uint32_t base_addr_low;
    uint32_t base_addr_high;
    uint32_t length_low;
    uint32_t length_high;
    uint32_t type;
} __attribute__((packed)) mmapEntry_t;

// 声明全局的 multiboot_t * 指针
extern MultiBoot_t* GlbMbootPtr;//GlbMbootPtr保存至main.c
extern MultiBoot_t* TempMbootPtr;//前分页的临时multiboot_t * 指针,来自/boot/boot.s
#endif