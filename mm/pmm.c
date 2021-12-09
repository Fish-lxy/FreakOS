#include "types.h"
#include "console.h"
#include "multiboot.h"
#include "debug.h"
#include "pmm.h"
#include "pmmanager.h"
#include "vmm.h"
#include "list.h"


static void getMemInfo();//获取物理内存信息
static void pmmanagerInit();//初始化内存管理器
static void freeAreaInit();
static void pagesInit();//设置Pages数组，将数组中的所有Page设为reserved
static void memmapInit(PageFrame_t* base, size_t n);

static PMManager_t Manager;//物理内存管理器
uint32_t PhyMemSize;//物理内存字节大小
uint32_t PhyMemEnd;//可用物理内存结尾

uint32_t PageCount;// 所有的物理内存页框数量
PageFrame_t* Pages;//Page结构保存起始虚拟地址
uint32_t FreeMemStart;//可用内存起始物理地址
FreeList FreeArea;//空闲区域链表

void initPMM() {
    //printk("Init PMM...\n");

    getMemInfo(); //获得物理内存的大小
    pmmanagerInit();
    freeAreaInit();
    pagesInit();
    memmapInit(pa2page(FreeMemStart), (PhyMemEnd - FreeMemStart) / PMM_PGSIZE);

    printk(" Free Memory: %dKB~%dMB\n", (PhyMemEnd - FreeMemStart) / 1024, (PhyMemEnd - FreeMemStart) / 1024 / 1024);

}



void pmmanagerInit() {
    PMManager_t* m = get_FF_PMManager();
    Manager.alloc_pages = m->alloc_pages;
    Manager.free_area_init = m->free_area_init;
    Manager.free_pages = m->free_pages;
    Manager.memmap_init = m->memmap_init;
}
void getMemInfo() {
    uint32_t mmap_addr = GlbMbootPtr->mmap_addr + KERNEL_OFFSET;//分页后需要加上偏移
    uint32_t mmap_length = GlbMbootPtr->mmap_length;
    for (mmapEntry_t* mmap = (mmapEntry_t*) mmap_addr;(uint32_t) mmap < mmap_addr + mmap_length;mmap++) {
        //如果是可用内存而且大于1MB区域
        //按照协议，type == 1 表示为可用内存，其它数字指保留区域
        //只使用物理内存中1MB以上的可用空间
        if (mmap->type == 1 && mmap->base_addr_low == 0x100000) {
            //获取内存大小，仅支持32位，只取了低32位长度
            PhyMemSize = (uint32_t) mmap->base_addr_low + mmap->length_low;
            PhyMemEnd = (uint32_t) mmap->base_addr_low + mmap->length_low; //获取可用内存结束地址
        }
    }
}
void pagesInit() {
    uint32_t end = kern_end;
    //Page起始于内核结束后新的一页
    Pages = (PageFrame_t*) ROUNDUP((void*) end, PMM_PGSIZE);

    PageCount = PhyMemSize / PMM_PGSIZE;
    //先将所有页都设定为保留
    for (uint32_t i = 0;i < PageCount;i++) {
        Pages[i].flags = 0;
        Pages[i].flags = SetBitOne(Pages[i].flags, PGP_reserved);
    }
    //空闲内存起始于Pages数组结束后新的一页
    FreeMemStart = ROUNDUP((void*) (&(Pages[PageCount])), PMM_PGSIZE) - KERNEL_OFFSET;

}
void freeAreaInit() {
    Manager.free_area_init();
}
void memmapInit(PageFrame_t* base, size_t n) {
    Manager.memmap_init(base, n);
}
PageFrame_t* allocPhyPages(size_t n) {
    return Manager.alloc_pages(n);
}
void freePhyPages(PageFrame_t* base, size_t n) {
    Manager.free_pages(base, n);
}
