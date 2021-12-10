#ifndef VMM_H
#define VMM_H
#include "types.h"
#include "pmm.h"
#include "idt.h"
#include "multiboot.h"

//Virtual Memory Management 虚拟内存管理

#define VMM_PGSIZE 0x1000 //页大小

#define VMM_PAGE_PERSENT 0x1
#define VMM_PAGE_WRITEABLE 0x2
#define VMM_PAGE_USER 0x4

#define VMM_PAGE_MASK 0xFFFFF000

//1PGD包含1024个PTE,映射4MB内存地址
//1PTE映射12位/4KB内存地址

//获得一个虚拟地址的页目录项
#define VMM_PGD_INDEX(x) (((x) >> 22) & 0x3FF)
//获得一个虚拟地址的页表项
#define VMM_PTE_INDEX(x) (((x) >> 12) & 0x3FF)
//获得一个虚拟地址的页内偏移
#define VMM_OFFSET_INDEX(x) ((x) & 0xFFF)

#define VMM_PGD_SIZE (VMM_PGSIZE/sizeof(PTE_t)) //1024
#define VMM_PTE_SIZE (VMM_PGSIZE/sizeof(uint32_t)) //1024
#define VMM_PGD_COUNT (MAX_PMM_MEM_SIZE/1024/4096) //256

typedef uint32_t PGD_t; //页目录类型
typedef uint32_t PTE_t; //页表类型

typedef struct  mm_struct_t
{
    /* data */
}mm_struct_t;


void initVMM();
void switchPGD(uint32_t pd); //切换页表

void invaildate(uint32_t addr);//刷新页表缓存，使包含addr的页对应的TLB项失效
void VMM_map(PGD_t* pgd_now, uint32_t vaddr, uint32_t paddr, uint32_t flags); //将物理地址映射到虚拟地址
void VMM_unmap(PGD_t* pgd_now, uint32_t vaddr);//取消映射
uint32_t VMM_getMapping(PGD_t* pgd_now, uint32_t vaddr, uint32_t* paddr);//获取映射信息
void pageFault(InterruptFrame_t* regs); //页中断处理，实现在/mm/page_fault.c

#endif