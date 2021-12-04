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

#endif
