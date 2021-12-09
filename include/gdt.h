#ifndef GDT_H_
#define GDT_H

#include "types.h"

// 全局描述符类型
typedef
struct GDT_Entry {
    uint16_t limit_low;     // 段界限   15 ～ 0
    uint16_t base_low;      // 段基地址 15 ～ 0
    uint8_t  base_middle;   // 段基地址 23 ～ 16
    uint8_t  access;        // 段存在位、描述符特权级、描述符类型、描述符子类别
    uint8_t  granularity;   // 其他标志、段界限 19 ～ 16
    uint8_t  base_high;     // 段基地址 31 ～ 24
} __attribute__((packed)) GDT_Entry;

// GDTR
typedef
struct GDT_Ptr {
    uint16_t limit;     // 全局描述符表限长
    uint32_t base;      // 全局描述符表 32 位基地址
} __attribute__((packed)) GDT_Ptr;

// 初始化全局描述符表
void initGDT();

// GDT 加载到 GDTR(汇编实现：krnl/gdt_s.s) 
extern void flushGDT(uint32_t);

/* global segment number */
#define SEG_KTEXT   1
#define SEG_KDATA   2
#define SEG_UTEXT   3
#define SEG_UDATA   4
#define SEG_TSS     5

/* global descrptor numbers */
#define GD_KTEXT    ((SEG_KTEXT) << 3)      // kernel text 0x8
#define GD_KDATA    ((SEG_KDATA) << 3)      // kernel data 0x10 16
#define GD_UTEXT    ((SEG_UTEXT) << 3)      // user text 0x18 24
#define GD_UDATA    ((SEG_UDATA) << 3)      // user data 0x20  32
#define GD_TSS      ((SEG_TSS) << 3)        // task segment selector 0x28 40

#define DPL_KERNEL  (0)
#define DPL_USER    (3)

#define KERNEL_CS   ((GD_KTEXT) | DPL_KERNEL) //0x8
#define KERNEL_DS   ((GD_KDATA) | DPL_KERNEL) //0x10 16
#define USER_CS     ((GD_UTEXT) | DPL_USER) //0x1B 27
#define USER_DS     ((GD_UDATA) | DPL_USER) //0x23 35

#endif
