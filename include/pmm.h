#ifndef PMM_H
#define PMM_H
#include "types.h"
#include "multiboot.h"
#include "list.h"
#include "debug.h"
#include "vmm.h"

// Physical Memory Management 物理内存管理(PMM)

extern uint8_t kern_start[];//由链接器提供内核的起始虚拟地址(/script/kernel.ld)
extern uint8_t kern_end[];
//物理内存字节大小
extern uint32_t PMM_PhyMemSize;
extern uint32_t PMM_PhyMemEnd;
//物理页框数量

extern uint32_t PMM_stack_PhyPageCount;

//内核栈大小
#define STACK_SIZE 8192
//支持的最大物理内存(1GB)
#define PMM_MEM_MAX_SIZE 0x40000000 //1GB
//物理页框大小(4KB)
#define PMM_PGSIZE 0x1000
//物理页面数量
#define PMM_PAGE_MAX_COUNT (PMM_MEM_MAX_SIZE/PMM_PGSIZE)
//页面掩码
#define PMM_PAGE_MASK 0xFFFFF000

typedef struct PageFrame_t
{
    //flag标志字段
    //位0表示保留内存，用来表示不可用的内存和已被内核占用的内存
    //位1表示管理位，表示加入FreeArea链表，可以被申请的内存
    uint32_t flags;
    //位1被设置为1时，此字段表示此块空闲内存包含的空闲块数量
    uint32_t property;
    uint32_t ref;
    //uint32_t addr;    
    list_ptr_t ptr;
}PageFrame_t;

#define PG_reserved 0x1
#define PG_property 0x2

#define PGP_reserved 0
#define PGP_property 1

typedef struct FreeList
{
    list_ptr_t ptr;
    uint32_t free_page_count;
}FreeList;

extern PageFrame_t* Pages;
extern FreeList FreeArea;
extern uint32_t PMM_PageCount;

void initPMM();
uint32_t allocPhyPage();
void freePhyPage(uint32_t p);


#define le2page(le, member)                 \
    to_struct((le), PageFrame_t, member)

//由页框地址计算出页框是第几页
static inline uint32_t page2pagen(PageFrame_t *page){
    return page - Pages;
}
static inline uint32_t page2pa(PageFrame_t *page){
    return page2pagen(page) << 12;
}
//由物理地址计算出该地址属于哪一页
static inline PageFrame_t* pa2page(uint32_t pa) {
    if((pa >> 12) > PMM_PageCount)
        panic("pa is invalid!");
    return &Pages[(pa >> 12)];
}

static inline printPage(PageFrame_t *p){
    printk("----------------------\n");
    printk("pageAddr:0x%08X\n",p);
    printk("flag:%d property:%d\n",p->flags,p->property);
    printk("pageAllocAddr:0x%08X\n",page2pa(p));
    printk("----------------------\n");
}
// static inline PageFrame_t* va2page(uint32_t va) {
//     if(((va - KERNEL_OFFSET) >> 12) > PMM_PageCount)
//         panic("va is invalid!");
//     return &Pages[((va - KERNEL_OFFSET) >> 12)];
// }

#endif