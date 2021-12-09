#include "types.h"
#include "pmm.h"
#include "vmm.h"

void* kmalloc(uint32_t bytes) {
    int pagen = ROUNDUP(bytes, 4096) / PMM_PGSIZE;
    PageFrame_t* paddr = allocPhyPages(pagen);
    return (void*) (page2pa(paddr) + KERNEL_OFFSET);

}
void kfree(uint32_t* vaddr, uint32_t bytes) {
    int pagen = ROUNDUP(bytes, 4096) / PMM_PGSIZE;
    freePhyPages(pa2page(vaddr - KERNEL_OFFSET), pagen);
}