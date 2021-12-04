#include "types.h"
#include "console.h"
#include "multiboot.h"
#include "debug.h"
#include "pmm.h"
#include "vmm.h"
#include "list.h"

static void pageinit();//设置Pages数组，将数组中的所有Page设为reserved

void get_mem_info();
void free_area_init();
void memmap_init(PageFrame_t* base, size_t n);
PageFrame_t* alloc_pages(size_t n);

//static uint32_t PMM_stack[PMM_PAGE_MAX_COUNT + 1]; //物理内存管理栈
//static uint32_t PMM_stackTop;
//uint32_t PMM_stack_PhyPageCount; //物理页框数量

uint32_t PMM_PhyMemSize;//物理内存字节大小
uint32_t PMM_PhyMemEnd;//可用物理内存结尾

uint32_t PMM_PageCount;// 所有的物理内存页框数量
PageFrame_t* Pages;//Page结构保存起始虚拟地址
uint32_t FreeMemStart;//可用内存起始物理地址
FreeList FreeArea;

void initPMM() {
    //consoleWriteColor("Init Physical Memory Management...\n", TC_black, TC_light_blue);
    get_mem_info(); //获得物理内存的大小
    free_area_init();
    pageinit();
    memmap_init(pa2page(FreeMemStart), (PMM_PhyMemEnd - FreeMemStart) / PMM_PGSIZE);

    printk(" Free Memory: 0x%X, %dKB, %dMB\n", PMM_PhyMemSize, PMM_PhyMemSize / 1024, PMM_PhyMemSize / 1024 / 1024);
    printk(" kernelEnd: 0x%08X\n", kern_end);
    printk(" PhyEnd: 0x%08X\n", PMM_PhyMemEnd);
    printk(" PagesStart: 0x%X,PMM_PageCount: %d\n", (uint32_t) Pages, PMM_PageCount);
    printk(" PagesEnd: 0x%X\n", &(Pages[PMM_PageCount]));
    printk(" PagesAllSize: %dB\n", sizeof(PageFrame_t) * PMM_PageCount);
    printk(" FreeMemStart: 0x%08X\n", FreeMemStart);
    printk(" FreeAreaSize: %d FreePageCount:%d\n", PMM_PhyMemEnd - FreeMemStart, (PMM_PhyMemEnd - FreeMemStart) / PMM_PGSIZE);
    printk(" free_page_count:%d\n", FreeArea.free_page_count);

    // int i = 123;
    // i = SetBitOne(i, 2);
    // i = SetBitZero(i, 2);
    // printk("%d\n", i);

    // int a0 = 0, a1 = 0, a2 = 0;
    // for (int i = 0;i < PMM_PageCount;i++) {
    //     if (Pages[i].flags == 0)
    //         ++a0;
    //     if (Pages[i].flags == 1)
    //         ++a1;
    //     if (Pages[i].flags == 2)
    //         ++a2;
    // }
    // printk("00:%d 01:%d 10:%d\n", a0, a1, a2);

    list_ptr_t* le1 = &(FreeArea.ptr);
    le1 = listGetNext(le1);
    PageFrame_t* page1 = le2page(le1, ptr);
    printk(" FirstPageProperty:%d\n", page1->property);

    PageFrame_t *p1 = alloc_pages(1);
    printPage(p1);
    

    list_ptr_t* le2 = &(FreeArea.ptr);
    le2 = listGetNext(le2);
    PageFrame_t* page2 = le2page(le2, ptr);
    printk(" FirstPageProperty:%d\n", page2->property);

}
uint32_t allocPhyPage() {

}
void freePhyPage(uint32_t p) {

}

void get_mem_info() {
    uint32_t mmap_addr = GlbMbootPtr->mmap_addr + KERNEL_OFFSET;//分页后需要加上偏移
    uint32_t mmap_length = GlbMbootPtr->mmap_length;
    for (mmapEntry_t* mmap = (mmapEntry_t*) mmap_addr;(uint32_t) mmap < mmap_addr + mmap_length;mmap++) {
        //如果是可用内存而且大于1MB区域
        //按照协议，type == 1 表示为可用内存，其它数字指保留区域
        //只使用物理内存中1MB以上的可用空间
        if (mmap->type == 1 && mmap->base_addr_low == 0x100000) {
            //获取内存大小，仅支持32位，只取了低32位长度
            PMM_PhyMemSize = (uint32_t) mmap->base_addr_low + mmap->length_low;
            PMM_PhyMemEnd = (uint32_t) mmap->base_addr_low + mmap->length_low; //获取可用内存结束地址
        }
    }
}
void free_area_init() {
    initList(&FreeArea.ptr);
    FreeArea.free_page_count = 0;
}
void pageinit() {
    uint32_t end = kern_end;
    //Page起始于内核结束后新的一页
    Pages = (PageFrame_t*) ROUNDUP((void*) end, PMM_PGSIZE);

    PMM_PageCount = PMM_PhyMemSize / PMM_PGSIZE;
    //所有页都设定为保留
    for (uint32_t i = 0;i < PMM_PageCount;i++) {
        Pages[i].flags = 0;
        Pages[i].flags = SetBitOne(Pages[i].flags, PGP_reserved);
        //Pages[i].flags |= PG_reserved;
    }
    //空闲内存起始于Pages数组结束后新的一页
    FreeMemStart = ROUNDUP((void*) (&(Pages[PMM_PageCount])), PMM_PGSIZE) - KERNEL_OFFSET;

}
void memmap_init(PageFrame_t* base, size_t n) {
    assert(n > 0, "n must greater than 0!");
    PageFrame_t* p = base;
    for (;p != base + n;p++) {
        if (p->flags != 0) {
            p->flags = 0;
            p->property = 0;
            p->ref = 0;
        }
    }
    base->property = n;
    base->flags = SetBitOne(base->flags, PGP_property);
    //base->flags |= PG_property;
    FreeArea.free_page_count += n;
    listAdd(&(FreeArea.ptr), &(base->ptr));
}
PageFrame_t* alloc_pages(size_t n) {
    assert(n > 0, "n is must greater than 0!");
    if (n > FreeArea.free_page_count)
        return NULL;

    PageFrame_t* page = NULL;
    PageFrame_t* p = NULL;
    list_ptr_t* le = &(FreeArea.ptr);
    //寻找第一个匹配的空闲块
    while ((le = listGetNext(le)) != &(FreeArea.ptr)) {
        p = le2page(le, ptr);
        if (p->property >= n) {
            page = p;
            break;
        }
    }
    //重新组织空闲块
    if (page != NULL) {
        if (page->property > n) {
            PageFrame_t* p = page + n;
            p->property = page->property - n;
            p->flags = SetBitOne(p->flags, PGP_property);
            //p->flags |= PG_property;
            listAdd(&(page->ptr), &(p->ptr));
        }
        listDel(&(page->ptr));
        FreeArea.free_page_count -= n;
        page->flags = SetBitZero(page->flags, PGP_property);
    }
    return page;

}