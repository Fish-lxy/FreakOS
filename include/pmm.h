#ifndef PMM_H
#define PMM_H
#include "types.h"
#include "multiboot.h"
#include "list.h"
#include "debug.h"
#include "vmm.h"

// Physical Memory Management 物理内存管理(PMM)


//物理内存字节大小
extern uint32_t PhyMemSize;
extern uint32_t PhyMemEnd;

//支持的最大物理内存(1GB)
#define MAX_PMM_MEM_SIZE 0x40000000 //1GB
//物理页框大小(4KB)
#define PMM_PGSIZE 0x1000
//页面掩码
#define PMM_PAGE_MASK 0xFFFFF000

#define PG_reserved 0x1
#define PG_property 0x2

#define PGP_reserved 0
#define PGP_property 1

typedef struct PageBlock_t
{
    //flag标志字段
    //位0表示保留内存，用来表示不可用的内存和已被内核占用的内存
    //位1表示管理位，表示加入FreeArea链表，可以被申请的内存
    uint32_t flags;
    //property内存分配器管理字段
    //位1被设置为1时，此字段表示此块空闲内存包含的空闲块数量
    uint32_t property;
    //引用数量
    uint32_t ref;
    //uint32_t addr;

    list_ptr_t ptr;
}PageBlock_t;

typedef struct FreeList
{
    list_ptr_t ptr;
    uint32_t free_page_count;
}FreeList;

extern PageBlock_t* Pages;
extern FreeList FreeArea;
extern uint32_t PageCount;
extern uint32_t FreeMemStart;

void initPMM();
PageBlock_t* allocPhyPages(size_t n);
void freePhyPages(PageBlock_t* base, size_t n);
uint32_t getFreeMem();



#define lp2page(le, member)                 \
    to_struct((le), PageBlock_t, member)

//由PageFrame_t地址计算出页框是第几页
static inline uint32_t page2pagen(PageBlock_t* page) {
    return page - Pages;
}
static inline uint32_t page2pa(PageBlock_t* page) {
    return page2pagen(page) << 12;
}
//由物理地址计算出该地址属于哪一页
static inline PageBlock_t* pa2page(uint32_t pa) {
    if ((pa >> 12) > PageCount)
        panic("pa is invalid!");
    return &Pages[(pa >> 12)];
}

// static inline printPage(PageBlock_t* p) {
//     printk("pageAddr:0x%08X ", p);
//     printk("flag:%d property:%d  ", p->flags, p->property);
//     printk("pageAllocAddr:0x%08X\n", page2pa(p));
// }

// static inline printPageList() {
//     printk("PageList:\n");
//     list_ptr_t* lp = &(FreeArea.ptr);
//     while ((lp = listGetNext(lp)) != &(FreeArea.ptr)) {
//         PageBlock_t* p = lp2page(lp, ptr);
//         printPage(p);
//     }
// }
#endif