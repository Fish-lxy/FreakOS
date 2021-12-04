#include "gdt.h"
#include "string.h"
#include "types.h"
#include "console.h"

// 全局描述符表长度
#define GDT_LENGTH 5

// 全局描述符表定义
GDT_Entry gdtEntries[GDT_LENGTH];

// GDTR
GDT_Ptr gdtPtr;

static void GDT_SetGate(int32_t num, uint32_t base,
    uint32_t limit, uint8_t access, uint8_t gran);

// 声明内核栈地址
extern uint32_t stack;

// 初始化全局描述符表
void initGDT() {
    //consoleWriteColor("Init Global Descriptor Table...", TC_black, TC_light_blue);
    // 全局描述符表界限 e.g. 从 0 开始，所以总长要 - 1
    gdtPtr.limit = sizeof(GDT_Entry) * GDT_LENGTH - 1;
    gdtPtr.base = (uint32_t) &gdtEntries;

    // 采用 Intel 平坦模型
    GDT_SetGate(0, 0, 0, 0, 0);   // 按照 Intel 文档要求，第一个描述符必须全 0
    GDT_SetGate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);     // 指令段
    GDT_SetGate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);     // 数据段
    GDT_SetGate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);     // 用户模式代码段
    GDT_SetGate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);     // 用户模式数据段

    // 加载全局描述符表地址到 GPTR 寄存器
    flushGDT((uint32_t) &gdtPtr);
    //consoleWriteColor("OK\n", TC_black, TC_yellow);
}

// 构造全局描述符表，根据下标构造
// 参数分别是 数组下标、基地址、限长、访问标志，其它访问标志
static void GDT_SetGate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdtEntries[num].base_low = (base & 0xFFFF);
    gdtEntries[num].base_middle = (base >> 16) & 0xFF;
    gdtEntries[num].base_high = (base >> 24) & 0xFF;

    gdtEntries[num].limit_low = (limit & 0xFFFF);
    gdtEntries[num].granularity = (limit >> 16) & 0x0F;

    gdtEntries[num].granularity |= gran & 0xF0;
    gdtEntries[num].access = access;
}