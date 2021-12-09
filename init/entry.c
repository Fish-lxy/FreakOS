#include "types.h"
#include "console.h"
#include "debug.h"
#include "gdt.h"
#include "idt.h"
#include "timer.h"
#include "multiboot.h"
#include "pmm.h"
#include "vmm.h"
//内核启动入口，初始化页表并开启分页，然后转到main

//内核栈大小
#define STACK_SIZE 8192

extern void main();//内核初始化入口

MultiBoot_t* GlbMbootPtr;
int8_t KernelStack[STACK_SIZE]; //内核栈

//内核临时页表和页目录的地址
__attribute__((section(".temp.data"))) PGD_t* PGD_temp = (PGD_t*) 0x1000;
__attribute__((section(".temp.data"))) PGD_t* PTE_low = (PGD_t*) 0x2000;
__attribute__((section(".temp.data"))) PGD_t* PTE_high = (PGD_t*) 0x3000;

__attribute__((section(".temp.text")))
void kernel_entry() {
	mmapTempVMPage();
	enablePaging();
	//切换内核栈
	uint32_t kernel_stack_top = ((uint32_t) KernelStack + STACK_SIZE) & 0xFFFFFFF0;
	asm volatile(
		"mov %0, %%esp\n\t"
		"xor %%ebp, %%ebp"
		::"r"(kernel_stack_top)
		);
	//更新MutiBoot指针位置
	GlbMbootPtr = (MultiBoot_t*) ((uint32_t) TempMbootPtr + KERNEL_OFFSET);
	//调用内核初始化入口
	main();
}

__attribute__((section(".temp.text")))
void mmapTempVMPage() { //设置临时页表
	//页目录项内容 = (页表起始物理地址 & ~0x0FFF) | PTE_U | PTE_W | PTE_P
	PGD_temp[0] = (uint32_t) PTE_low | VMM_PAGE_PERSENT | VMM_PAGE_WRITEABLE;
	//768
	PGD_temp[VMM_PGD_INDEX(KERNEL_OFFSET)] = (uint32_t) PTE_high | VMM_PAGE_PERSENT | VMM_PAGE_WRITEABLE;

	//页表项内容 = (pa << 12) | PTE_P | PTE_W
	//映射内核虚拟地址物理地址前4MB
	for (int i = 0;i < 1024;i++) {
		PTE_low[i] = (i << 12) | VMM_PAGE_PERSENT | VMM_PAGE_WRITEABLE;
	}
	//映射虚拟地址0xC0000000-0xC0400000到物理地址前4MB
	for (int i = 0;i < 1024;i++) {
		PTE_high[i] = (i << 12) | VMM_PAGE_PERSENT | VMM_PAGE_WRITEABLE;
	}
	asm volatile(
		"mov %0, %%cr3"
		::"r"(PGD_temp)//设置临时页表
		);
}
__attribute__((section(".temp.text")))
void enablePaging() { //打开分页
	uint32_t cr0;
	asm volatile(
		"mov %%cr0, %0"
		:"=r"(cr0)
		);
	cr0 |= 0x80000000;
	asm volatile(
		"mov %0, %%cr0"
		::"r"(cr0)
		);
}