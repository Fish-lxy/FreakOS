#include "kmalloc.h"
#include "debug.h"
#include "intr_sync.h"
#include "mm.h"
#include "pmm.h"
#include "types.h"

static int getBucketSlot(int size);
static Bucket_t *allocBucket();
static int32_t initBucket(Bucket_t *bk, int size);
static void *doKmalloc(uint32_t size);
static int32_t doKfree(void *addr, uint32_t size);

void *dokmalloc2(uint32_t bytes);
void dokfree2(uint32_t *vaddr, uint32_t bytes);

// 为内核数据结构申请内存，分配的内存地址带3GB偏移
void *kmalloc(uint32_t bytes) {
    void *vaddr = dokmalloc2(bytes);
#if DEBUG_KMALLOC_SPRINTK == 1
    sprintk("内核申请内存:0x%08X,%dB\n", vaddr, bytes);
#endif
    if (vaddr == NULL) {
        sprintk("申请内存失败!\n");
    }
    return vaddr;
    // return doKmalloc(bytes);
}
void kfree(void *vaddr, uint32_t bytes) {
    dokfree2(vaddr, bytes);
#if DEBUG_KMALLOC_SPRINTK == 1
    sprintk("内核释放内存:0x%08X,%dB\n", vaddr, bytes);
#endif
    // uint32_t err = doKfree(vaddr, bytes);
    // if (err == -1) {
    //     panic("kfree: error size");
    // } else if (err == -2) {
    //     panic("kfree: addr is not available!");
    // }
}

void *dokmalloc2(uint32_t bytes) {
    //将字节数对齐到4K大小，计算申请页数
    int pagen = ROUNDUP(bytes, 4096) / PGSIZE;
    //申请物理页
    PhyPageBlock_t *paddr = allocPhyPages(pagen);
    //将链表条目转换为物理页地址
    return (void *)(page2pa(paddr) + KERNEL_BASE);
}

void dokfree2(uint32_t *vaddr, uint32_t bytes) {
    int pagen = ROUNDUP(bytes, 4096) / PGSIZE;
    // printk("free:0x%08X\n",vaddr);
    uint32_t vaddr2 = (uint32_t)vaddr - KERNEL_BASE;
    // printk("free:0x%08X\n",vaddr2);
    freePhyPages(pa2page(vaddr2), pagen);
}

Bucket_t BucketTable[] = {{32, 0, NULL, NULL},   {64, 0, NULL, NULL},
                          {128, 0, NULL, NULL},  {256, 0, NULL, NULL},
                          {512, 0, NULL, NULL},  {1024, 0, NULL, NULL},
                          {2048, 0, NULL, NULL}, {4096, 0, NULL, NULL}};
Bucket_t BucketFreeList = {
    0,
};
int InitBucketCount = 0;

// 计算size大小的内存应该对应的桶号
int getBucketSlot(int size) {
    int n, i;

    if (size <= 0)
        return -1;
    n = 16;
    i = 0;
    while ((n <<= 1) <= PGSIZE) {
        if (size <= n) {
            return i;
        }
        i++;
    }
    return -1;
}

// 申请一个空桶，所有的空桶都保存在BucketFreeList中
// 若BucketFreeList中无空桶，申请一页内存并将其分割
Bucket_t *allocBucket() {
    bool lock;
    Bucket_t *p, *q, *bkfreelist_head;
    uint32_t page_addr, page_end_addr;

    bkfreelist_head = &BucketFreeList;
    // 若无空桶，申请一页内存并将其分割
    if (BucketFreeList.next == NULL) {
        page_addr = page2pa(allocPhyPages(1)) + KERNEL_BASE;
        printk("kmalloc:0x%08X\n", page_addr);
        page_end_addr = page_addr + PGSIZE;

        q = &BucketFreeList;
        p = (Bucket_t *)page_addr;

        intr_save(lock);
        while ((uint32_t)p < page_end_addr) {
            p++;
            q->next = p;
            p->next = NULL;
            q = p;
        }
        intr_restore(lock);
    }

    // 获取一个空桶
    p = BucketFreeList.next;
    BucketFreeList.next = p->next;

    return p;
}

int32_t initBucket(Bucket_t *bk, int size) {
    uint32_t page_addr;
    BkEntry_t *q, *p;

    page_addr = page2pa(allocPhyPages(1)) + KERNEL_BASE;

    bk->page = page_addr;
    bk->size = size;

    bk->entry = q = (BkEntry_t *)(page_addr);
    // 按size平均分割内存页，每一个块为一个BucketEntry_t结构，下面的循环使每个块链接起来
    for (int i = 0; i < PGSIZE / size; i++) {
        p = (BkEntry_t *)(page_addr + i * size); // p移动向下一个块
        q->bke_next = p;    // 使p的前一个块q与p链接
        p->bke_next = NULL; // 清空p的next
        q = p;              // 指针q向p移动
    }
    return 0;
}
void *doKmalloc(uint32_t size) {
    if (size == 0) {
        return 0;
    }
    if (size >= PGSIZE) {
        int pagen = ROUNDUP(size, 4096) / PGSIZE;
        PhyPageBlock_t *paddr = allocPhyPages(pagen);
        return (void *)(page2pa(paddr) + KERNEL_BASE);
    }

    uint32_t index = 0;
    Bucket_t *bk, *bk_head;
    BkEntry_t *be;

    index = getBucketSlot(size);
    if (index < 0) {
        panic("kmalloc: error size");
    }

    bk = bk_head = &BucketTable[index];
    size = bk_head->size;

    while (1) {
        while (bk != NULL) {
            if (bk->entry != NULL) {
                be = bk->entry;
                bk->entry = be->bke_next;
                return (void *)be;
            }
            bk = bk->next;
        }

        InitBucketCount++;
        bk = allocBucket();
        initBucket(bk, size);
        printk("get new bk:0x%08X\n", bk);
        ;

        bk->next = bk_head->next;
        bk_head->next = bk;
    }
}
int32_t doKfree(void *vaddr, uint32_t size) {
    if (size == 0 || vaddr == NULL) {
        return 0;
    }
    if (size >= PGSIZE) {
        int pagen = ROUNDUP(size, 4096) / PGSIZE;
        freePhyPages(pa2page(vaddr - KERNEL_BASE), pagen);
    }

    uint32_t page_addr;
    uint32_t index = 0;
    Bucket_t *bk, *bk_head;
    BkEntry_t *be;

    index = getBucketSlot(size);
    if (index < 0) {
        return -1;
    }

    // 计算vaddr对应的页的首地址
    // 等同于先将vaddr右移12位，再左移12位，清空末尾12位
    // page_addr = ((uint32_t)vaddr >> 12) << 12;
    page_addr = page2pa(pa2page(vaddr - KERNEL_BASE)) + KERNEL_BASE;

    bk = bk_head = &BucketTable[index];
    size = bk_head->size;
    be = (BkEntry_t *)vaddr;

    while (bk != NULL) {
        if (bk->page == page_addr) {
            be->bke_next = bk->entry;
            bk->entry = be;
            return 0;
        }
        bk = bk->next;
    }
    return -2;
}

void printAllBucket() {
    // int i = 0;
    Bucket_t *bk = &(BucketTable[0]);
    for (int i = 0; i < 8; i++) {
        bk = &(BucketTable[i]);
        printk("%d ", bk->size);
        for (BkEntry_t *be = bk->entry; be != NULL; be = be->bke_next) {
            printk(" %08X ", be);
        }
        printk("\n");
    }
}
void testKmalloc() {
    printk("\n");
    uint32_t free_mem = 0;

    // uint32_t free_mem1 = getFreeMem();
    // printk("PMM:FreeMem:%dB, %dKB\n", free_mem1, free_mem1 / 1024);

    uint32_t *p = NULL;
    for (int i = 0; i < 4; i++) {
        p = doKmalloc(64);
        *p = 2147483647;
        if (*p != 2147483647) {
            printk("error!\n");
            printk("p:%08X *p=%u\n", p, *p);
            break;
        }
        printk("p:%08X *p=%d\n", p, *p);
        doKfree(p, 64);
    }
    printk("\n");
    // doKfree(p, 64);

    p = doKmalloc(16);
    // *p = 2147483647;
    // if (*p != 2147483647) {
    //     printk("error!\n");
    //     printk("p:%08X *p=%u\n", p, *p);
    // }
    // printk("p:%08X *p=%d\n", p, *p);
    // doKfree(p,16);

    PhyPageBlock_t *paddr = allocPhyPages(1);
    for (int i = 0; i < 32; i++) {
        paddr = allocPhyPages(1) + KERNEL_BASE;
    }
    uint32_t a = (uint32_t)(page2pa(paddr));
    printk("a:%08X\n", a);

    // printk("\n");
    // allocPhyPages(1);
    // for (int i = 0;i < 8;i++) {
    //     p = doKmalloc(64);
    //     *p = 2147483647;
    //     if (*p != 2147483647) {
    //         printk("error!\n");
    //         printk("p:%08X *p=%u\n", p, *p);
    //         break;
    //     }
    //     printk("p:%08X *p=%d\n", p, *p);

    // }

    // printAllBucket();
    // printk("InitBucketCount:%d\n", InitBucketCount);

    // uint32_t free_mem2 = getFreeMem();
    // printk("PMM:FreeMem:%dB, %dKB\n", free_mem2, free_mem2 / 1024);
    // printk("PMM:used:%dB %dKB\n", free_mem1 - free_mem2,
    //        (free_mem1 - free_mem2) / 1024);

    // while (1) {}
    printk("\n");
}