#include "types.h"
#include "string.h"
#include "debug.h"
#include "vmm.h"
#include "pmm.h"
#include "idt.h"

//正式的内核页表和页目录
static PGD_t kernelPGD[VMM_PGD_SIZE] __attribute__((aligned(VMM_PGSIZE)));
static PTE_t kernelPTE[VMM_PGD_COUNT][VMM_PTE_SIZE] __attribute__((aligned(VMM_PGSIZE)));

void initVMM() {
    consoleWriteColor("Init Virtual Memory Management...", TC_black, TC_light_blue);

    //0xC0000000在页目录的索引
    uint32_t kernelPTEFirstIdx = VMM_PGD_INDEX(KERNEL_OFFSET);
    for (uint32_t i = kernelPTEFirstIdx, j = 0;i < VMM_PGD_COUNT + kernelPTEFirstIdx;i++, j++) {
        //MMU需要物理地址，减去偏移
        kernelPGD[i] = ((uint32_t) kernelPTE[j] - KERNEL_OFFSET) | VMM_PAGE_PERSENT | VMM_PAGE_WRITEABLE;
    }

    uint32_t* pte = (uint32_t*) kernelPTE;
    for (int i = 0;i < VMM_PGD_COUNT * VMM_PTE_SIZE;i++) {
        pte[i] = (i << 12) | VMM_PAGE_PERSENT | VMM_PAGE_WRITEABLE;
    }
    uint32_t pgdKernelPhyAddr = (uint32_t) kernelPGD - KERNEL_OFFSET;
    interruptHandlerRegister(14, &pageFault);//设置页错误中断
    switchPGD(pgdKernelPhyAddr);

    consoleWriteColor("OK\n", TC_black, TC_yellow);
}
void switchPGD(uint32_t pd) {
    asm volatile(
        "mov %0, %%cr3"
        ::"r"(pd)
        );
}
void invaildate(uint32_t addr){
    asm volatile(
        "invlpg (%0)"::"a"(addr)
        );
}
void VMM_map(PGD_t* pgd_now, uint32_t vaddr, uint32_t paddr, uint32_t flags) {
    uint32_t pgd_index = VMM_PGD_INDEX(vaddr);
    uint32_t pte_index = VMM_PTE_INDEX(vaddr);
    PTE_t* pte = (PTE_t*) (pgd_now[pgd_index] & VMM_PAGE_MASK);

    if (pte == 0) {
        pte = (PTE_t*) allocPhyPage();
        pgd_now[pgd_index] = (uint32_t) pte | VMM_PAGE_PERSENT | VMM_PAGE_WRITEABLE;
        bzero(pte, VMM_PGSIZE);
    }
    else {
        pte = (PTE_t*) ((uint32_t) pte + KERNEL_OFFSET);
    }
    pte[pte_index] = (paddr & PMM_PAGE_MASK) | flags;//映射物理地址到PTE中
    //刷新页表缓存(TLB)
    invaildate(vaddr);

}
void VMM_unmap(PGD_t* pgd_now, uint32_t vaddr) {
    uint32_t pgd_index = VMM_PGD_INDEX(vaddr);
    uint32_t pte_index = VMM_PTE_INDEX(vaddr);
    PTE_t* pte = (PTE_t*) (pgd_now[pgd_index] & VMM_PAGE_MASK);

    if (pte == 0) {
        return;
    }

    pte = (PTE_t*) ((uint32_t) pte + KERNEL_OFFSET);
    pte[pte_index] = 0;
    
    invaildate(vaddr);
}
uint32_t VMM_getMapping(PGD_t* pgd_now, uint32_t vaddr, uint32_t *paddr) {
    uint32_t pgd_index = VMM_PGD_INDEX(vaddr);
    uint32_t pte_index = VMM_PTE_INDEX(vaddr);
    PTE_t* pte = (PTE_t*) (pgd_now[pgd_index] & VMM_PAGE_MASK);

    if (pte == 0) {
        return 0;
    }
    pte = (PTE_t*) ((uint32_t) pte + KERNEL_OFFSET);
    if (pte[pte_index] != 0 && paddr != 0) {
        *paddr = pte[pte_index] & VMM_PAGE_MASK;
        return 1;
    }
    return 0;
}