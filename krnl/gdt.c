#include "gdt.h"
#include "string.h"
#include "types.h"
#include "console.h"
#include "debug.h"

// 全局描述符表长度
#define GDT_LENGTH 6

// 全局描述符表定义
SegDesc GDT[GDT_LENGTH];

extern uint8_t KernelStack[]; // init/entry.c
TaskState TS;

// GDTR
GDT_Ptr gdtPtr;

static void GDT_SetNullSegGate(uint32_t n);
static void GDT_SetSegGate(uint32_t n, uint32_t type, uint32_t base, uint32_t limit, uint32_t dpl);
static void GDT_SetSegTSSGate(uint32_t n, uint32_t type, uint32_t base, uint32_t limit, uint32_t dpl);
static inline void flushTSS(uint16_t sel);

// 初始化全局描述符表
void initGDT() {
    //consoleWriteColor("Init Global Descriptor Table...", TC_black, TC_light_blue);
    // 全局描述符表界限 e.g. 从 0 开始，所以总长要 - 1
    gdtPtr.limit = sizeof(GDT) * GDT_LENGTH - 1;
    //gdtPtr.base = (uint32_t) &gdtEntries;
    gdtPtr.base = (uint32_t) GDT;

    printk("0x%08X\n", &GDT);

    TS.ts_esp0 = (uint32_t) KernelStack;
    TS.ts_ss0 = KERNEL_DS;

    // 采用 Intel 平坦模型
    GDT_SetNullSegGate(0);//第一个描述符必须全 0
    GDT_SetSegGate(1, STA_X | STA_R, 0x0, 0xFFFFFFFF, DPL_KERNEL);// 内核指令段
    GDT_SetSegGate(2, STA_W, 0x0, 0xFFFFFFFF, DPL_KERNEL);// 内核数据段
    GDT_SetSegGate(3, STA_X | STA_R, 0x0, 0xFFFFFFFF, DPL_USER);// 用户模式代码段
    GDT_SetSegGate(4, STA_W, 0x0, 0xFFFFFFFF, DPL_USER); // 用户模式数据段
    GDT_SetSegTSSGate(5, STS_T32A, &TS, sizeof(TS), DPL_KERNEL);
    //GDT_SetNullSegGate(5);//TSS

    // 加载全局描述符表地址到 GPTR 寄存器
    flushGDT((uint32_t) &gdtPtr);
    flushTSS(GD_TSS);
    //consoleWriteColor("OK\n", TC_black, TC_yellow);
}
// 构造全局描述符表，根据下标构造
static void GDT_SetNullSegGate(uint32_t n) {
    GDT[n].sd_lim_15_0 = 0;
    GDT[n].sd_base_15_0 = 0;
    GDT[n].sd_base_23_16 = 0;

    GDT[n].sd_type = 0;
    GDT[n].sd_s = 0;
    GDT[n].sd_dpl = 0;
    GDT[n].sd_present = 0;

    GDT[n].sd_lim_19_16 = 0;

    GDT[n].sd_avl = 0;
    GDT[n].sd_reserved = 0;
    GDT[n].sd_db = 0;
    GDT[n].sd_granularity = 0;

    GDT[n].sd_base_31_24 = 0;
}

static void GDT_SetSegGate(uint32_t n, uint32_t type, uint32_t base, uint32_t limit, uint32_t dpl) {
    GDT[n].sd_lim_15_0 = (limit >> 12) & 0xffff;
    GDT[n].sd_base_15_0 = (base) & 0xffff;
    GDT[n].sd_base_23_16 = (base >> 16) & 0xff;

    GDT[n].sd_type = type;
    GDT[n].sd_s = 1;
    GDT[n].sd_dpl = dpl;
    GDT[n].sd_present = 1;

    GDT[n].sd_lim_19_16 = (unsigned) (limit) >> 28;

    GDT[n].sd_avl = 0;
    GDT[n].sd_reserved = 0;
    GDT[n].sd_db = 1;
    GDT[n].sd_granularity = 1;

    GDT[n].sd_base_31_24 = (unsigned) (base) >> 24;
}
static void GDT_SetSegTSSGate(uint32_t n, uint32_t type, uint32_t base, uint32_t limit, uint32_t dpl) {
    GDT[n].sd_lim_15_0 = limit & 0xffff;
    GDT[n].sd_base_15_0 = (base) & 0xffff;
    GDT[n].sd_base_23_16 = (base >> 16) & 0xff;

    GDT[n].sd_type = type;
    GDT[n].sd_s = 0;
    GDT[n].sd_dpl = dpl;
    GDT[n].sd_present = 1;

    GDT[n].sd_lim_19_16 = (unsigned) (limit) >> 16;

    GDT[n].sd_avl = 0;
    GDT[n].sd_reserved = 0;
    GDT[n].sd_db = 1;
    GDT[n].sd_granularity = 0;

    GDT[n].sd_base_31_24 = (unsigned) (base) >> 24;
}
static inline void flushTSS(uint16_t sel) {
    asm volatile ("ltr %0" :: "r" (sel) : "memory");
}