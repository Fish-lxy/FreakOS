#include "console.h"
#include "debug.h"
#include "gdt.h"
#include "idt.h"
#include "mm.h"
#include "multiboot.h"
#include "pmm.h"
#include "timer.h"
#include "types.h"

// 内核启动入口，初始化页表并开启分页，然后转到main

// 内核栈大小
#define STACK_SIZE 8192

extern void main(); // 内核初始化入口

MultiBoot_t *GlbMbootPtr;
uint8_t KernelStack[STACK_SIZE]; // 内核栈

// 内核临时页表和页目录的首地址
// 页目录和页表地址要求必须 4K 对齐
// 内存的 0 ~ 640K 为空闲空间 使用0x1000开始的12KB空间来保存临时页表和临时页目录
__attribute__((section(".temp.data"))) PGD_t *PGD_temp = (PGD_t *)0x1000;//4KB处
__attribute__((section(".temp.data"))) PGD_t *PTE_low = (PGD_t *)0x2000;//8KB处
__attribute__((section(".temp.data"))) PGD_t *PTE_high = (PGD_t *)0x3000;//12KB处

__attribute__((section(".temp.text"))) void kernel_entry() {
    mmapTempVMPage();
    enablePaging();
    // 切换内核栈
    uint32_t kernel_stack_top =
        ((uint32_t)KernelStack + STACK_SIZE) & 0xFFFFFFF0;
    asm volatile("mov %0, %%esp\n\t"
                 "xor %%ebp, %%ebp" ::"r"(kernel_stack_top));
    // 更新MutiBoot指针位置
    GlbMbootPtr = (MultiBoot_t *)((uint32_t)TempMbootPtr + KERNEL_BASE);

    // 调用内核初始化入口
    main();
}

__attribute__((section(".temp.text"))) void mmapTempVMPage() { // 设置临时页表
    // 页目录项内容 = (页表起始物理地址 & ~0x0FFF) | PTE_U | PTE_W | PTE_P
    PGD_temp[0] = (uint32_t)PTE_low | PTE_PAGE_PERSENT | PTE_PAGE_WRITEABLE;
    // 768
    PGD_temp[PGD_INDEX(KERNEL_BASE)] =
        (uint32_t)PTE_high | PTE_PAGE_PERSENT | PTE_PAGE_WRITEABLE;

    // 页表项内容 = (pa << 12) | PTE_P | PTE_W
    // 映射内核虚拟地址物理地址前4MB
    for (int pagei = 0; pagei < 1024; pagei++) {
        PTE_low[pagei] = (pagei << 12) | PTE_PAGE_PERSENT | PTE_PAGE_WRITEABLE;
    }
    // 映射虚拟地址0xC0000000-0xC0400000到物理地址前4MB
    for (int pagei = 0; pagei < 1024; pagei++) {
        PTE_high[pagei] = (pagei << 12) | PTE_PAGE_PERSENT | PTE_PAGE_WRITEABLE;
    }
    asm volatile("mov %0, %%cr3" ::"r"(PGD_temp) // 设置临时页表
    );
}
__attribute__((section(".temp.text"))) void enablePaging() { // 打开分页
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    asm volatile("mov %0, %%cr0" ::"r"(cr0));
}