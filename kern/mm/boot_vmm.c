#include "debug.h"
#include "idt.h"
#include "kmalloc.h"
#include "mm.h"
#include "pmm.h"
#include "string.h"
#include "types.h"
#include "vmm.h"

// 内核页表和页目录
PGD_t *KernelPGD;
PTE_t *KernelPTE;

uint32_t kernel_cr3;

static void setBootKernelPGD();

void initKernelPageTable() {
    // printk("Init VMM...\n");
    printk("Setting new kernel page table...");

    KernelPGD = kmalloc(sizeof(PGD_t) * PGD_ECOUNT); // 4B*1024=4KB 1phypage
    KernelPTE = kmalloc(sizeof(PTE_t) * PGD_ECOUNT *
                        PGD_PHY_ECOUNT); // 4B*256*1024=1024KB=1MB 256phypage
    bzero(KernelPGD, sizeof(PGD_t) * PGD_ECOUNT);
    bzero(KernelPTE, sizeof(PTE_t) * PGD_ECOUNT * PGD_PHY_ECOUNT);

    setBootKernelPGD();

    intrHandlerRegister(14, &pageFaultCallback); // 设置页错误中断
    kernel_cr3 = (uint32_t)KernelPGD - KERNEL_BASE;
    switchPageDir(kernel_cr3);

    printk("OK\n");
}

// 设置正式内核页目录和页表的内容
static void setBootKernelPGD() {
    // 4GB虚拟地址空间页表共1024个页目录项，每个页目录项指向有1024个页表项
    uint32_t kernelPTEfirstidx = PGD_INDEX(
        KERNEL_BASE); // 获取虚拟地址0xC0000000在页目录的索引号 =3*256=768
    // printk("01");
    //  设置页目录项
    for (uint32_t i = kernelPTEfirstidx, j = 0; i < PGD_ECOUNT; i++, j++) {
        // 每个页目录项指向一个页表，一个页表有1024个项目
        KernelPGD[i] =
            ((uint32_t) & (KernelPTE[j * PTE_ECOUNT]) - KERNEL_BASE) |
            PTE_PAGE_PERSENT | PTE_PAGE_WRITEABLE;
    }
    // printk("02");
    //  设置所有页表项，使其指向物理地址
    uint32_t *pte = (uint32_t *)KernelPTE;
    for (int i = 0; i < PGD_ECOUNT * PGD_PHY_ECOUNT; i++) {
        pte[i] = (i << 12) | PTE_PAGE_PERSENT | PTE_PAGE_WRITEABLE;
    }
    // printk("03");
}

// 复制页表内核映射部分
void copy_kern_pgdir(PGD_t *to_pgdir) {
    uint32_t kernelPTEfirstidx =
        PGD_INDEX(KERNEL_BASE); // 获取虚拟地址0xC0000000在页目录的索引号
    for (uint32_t i = kernelPTEfirstidx, j = 0; i < PGD_ECOUNT; i++, j++) {
        // 每个页目录项指向一个页表，一个页表有1024个项目
        to_pgdir[i] = KernelPGD[i];
    }
}