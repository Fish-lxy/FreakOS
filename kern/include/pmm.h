#ifndef __PMM_H
#define __PMM_H
#include "debug.h"
#include "list.h"
#include "mm.h"
#include "multiboot.h"
#include "types.h"
#include "vmm.h"

// Physical Memory Management 物理内存管理(PMM)

// 物理内存字节大小
extern uint32_t PhyMemSize;
extern uint32_t PhyMemEnd;

#define PG_reserved 0x1
#define PG_property 0x2

#define PGP_reserved 0
#define PGP_property 1

typedef struct PhyPageBlock_t {
    // flag标志字段
    // 位0表示保留内存，用来表示不可用的内存和已被内核占用的内存
    // 位1表示管理位，表示加入FreeArea链表，可以被申请的内存
    uint32_t flags;
    // property内存分配器管理字段
    // 位1被设置为1时，此字段表示此块空闲内存包含的空闲块数量
    uint32_t property;
    // 引用数量
    uint32_t ref;
    // uint32_t addr;

    list_ptr_t ptr;
} PhyPageBlock_t;

#define lp2page(le, member) to_struct((le), PhyPageBlock_t, member)

typedef struct FreeList {
    list_ptr_t ptr;
    uint32_t free_page_count;
} FreeList;

extern PhyPageBlock_t *Pages;
extern FreeList FreeArea;
extern uint32_t PageCount;
extern uint32_t FreeMemStart;

void initPMM();
PhyPageBlock_t *allocPhyPages(size_t n);
void freePhyPages(PhyPageBlock_t *base, size_t n);
uint32_t getFreeMem();
void printFreeMem();

static inline int page_ref(PhyPageBlock_t *page) { return page->ref; }

static inline void set_page_ref(PhyPageBlock_t *page, int val) {
    page->ref = val;
}

static inline int page_ref_inc(PhyPageBlock_t *page) {
    page->ref += 1;
    return page->ref;
}

static inline int page_ref_dec(PhyPageBlock_t *page) {
    page->ref -= 1;
    return page->ref;
}

// 由PageFrame_t地址计算出页框是第几页
static inline uint32_t page2pagen(PhyPageBlock_t *page) { return page - Pages; }
static inline uint32_t page2pa(PhyPageBlock_t *page) {
    return page2pagen(page) << 12;
}
// 由物理地址计算出该地址属于哪一页
static inline PhyPageBlock_t *pa2page(uint32_t pa) {
    // printk("pa:0x%08X pa>>12:%d\n",pa,pa>>12);
    if ((pa >> 12) > PageCount)
        panic("pa is invalid!");
    return &Pages[(pa >> 12)];
}
// PPN 由物理地址计算出该地址属于第几页
static inline uint32_t pa2pagen(uint32_t pa) {
    if ((pa >> 12) > PageCount)
        panic("pa is invalid!");
    return pa >> 12;
}
// 内核虚拟地址转换为物理地址
static inline uint32_t kva2pa(uint32_t kva) {
    uint32_t _kva = kva;
    if (_kva < KERNEL_BASE) {
        panic("kva2pa err: arg is not less than KERNEL_BASE! ");
    }
    return _kva - KERNEL_BASE;
}
// 物理地址转换为内核虚拟地址
static inline uint32_t pa2kva(uint32_t pa) {
    uint32_t _pa = pa;
    uint32_t _m = pa2pagen(_pa);
    return pa + KERNEL_BASE;
}

static inline void *page2kva(PhyPageBlock_t *page) {
    return pa2kva(page2pa(page));
}

static inline PhyPageBlock_t *kva2page(void *kva) {
    return pa2page(kva2pa(kva));
}

static inline PhyPageBlock_t *pte2page(PTE_t pte) {
    if (!(pte & PTE_PAGE_PERSENT)) {
        panic("pte2page err: invalid pte!");
    }
    return pa2page(PTE_ADDR(pte));
}

static inline PhyPageBlock_t *pgd2page(PGD_t pgd) {
    return pa2page(PGD_ADDR(pgd));
}

// static inline printPage(PhyPageBlock_t* p) {
//     printk("pageAddr:0x%08X ", p);
//     printk("flag:%d property:%d  ", p->flags, p->property);
//     printk("pageAllocAddr:0x%08X\n", page2pa(p));
// }

// static inline printPageList() {
//     printk("PageList:\n");
//     list_ptr_t* lp = &(FreeArea.ptr);
//     while ((lp = listGetNext(lp)) != &(FreeArea.ptr)) {
//         PhyPageBlock_t* p = lp2page(lp, ptr);
//         printPage(p);
//     }
// }
#endif