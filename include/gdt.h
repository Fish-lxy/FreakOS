#ifndef GDT_H
#define GDT_H

#include "types.h"
#include "mm.h"


// GDTR
typedef
struct GDT_Ptr {
    uint16_t limit;     // 全局描述符表限长
    uint32_t base;      // 全局描述符表 32 位基地址
} __attribute__((packed)) GDT_Ptr;

// 段描述符
typedef
struct SegDesc {
    unsigned sd_lim_15_0 : 16;      // low bits of segment limit
    unsigned sd_base_15_0 : 16;     // low bits of segment base address
    unsigned sd_base_23_16 : 8;     // middle bits of segment base address

    unsigned sd_type : 4;           // segment type (see STS_ constants)
    unsigned sd_s : 1;              // 0 = system, 1 = application
    unsigned sd_dpl : 2;            // descriptor Privilege Level
    unsigned sd_present : 1;              // present

    unsigned sd_lim_19_16 : 4;      // high bits of segment limit

    unsigned sd_avl : 1;            // unused (available for software use)
    unsigned sd_reserved : 1;           // reserved
    unsigned sd_db : 1;             // 0 = 16-bit segment, 1 = 32-bit segment
    unsigned sd_granularity : 1;              // granularity: limit scaled by 4K when set

    unsigned sd_base_31_24 : 8;     // high bits of segment base address
} SegDesc;

typedef
struct TaskState {
    uint32_t ts_link;       // old ts selector
    uint32_t ts_esp0;      // stack pointers and segment selectors
    uint16_t ts_ss0;        // after an increase in privilege level
    uint16_t ts_padding1;
    uint32_t ts_esp1;
    uint16_t ts_ss1;
    uint16_t ts_padding2;
    uint32_t ts_esp2;
    uint16_t ts_ss2;
    uint16_t ts_padding3;
    uint32_t ts_cr3;       // page directory base
    uint32_t ts_eip;       // saved state from last task switch
    uint32_t ts_eflags;
    uint32_t ts_eax;        // more saved state (registers)
    uint32_t ts_ecx;
    uint32_t ts_edx;
    uint32_t ts_ebx;
    uint32_t ts_esp;
    uint32_t ts_ebp;
    uint32_t ts_esi;
    uint32_t ts_edi;
    uint16_t ts_es;         // even more saved state (segment selectors)
    uint16_t ts_padding4;
    uint16_t ts_cs;
    uint16_t ts_padding5;
    uint16_t ts_ss;
    uint16_t ts_padding6;
    uint16_t ts_ds;
    uint16_t ts_padding7;
    uint16_t ts_fs;
    uint16_t ts_padding8;
    uint16_t ts_gs;
    uint16_t ts_padding9;
    uint16_t ts_ldt;
    uint16_t ts_padding10;
    uint16_t ts_t;          // trap on task switch
    uint16_t ts_iomb;       // i/o map base address
} __attribute__((packed)) TaskState;

// 初始化全局描述符表
void initGDT();

// GDT 加载到 GDTR(汇编实现：krnl/gdt_s.s) 
extern void flushGDT(uint32_t);


#endif
