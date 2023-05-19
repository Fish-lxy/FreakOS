#ifndef __VMM_H
#define __VMM_H
#include "idt.h"
#include "list.h"
#include "mm.h"
#include "multiboot.h"
#include "pmm.h"
#include "sem.h"
#include "types.h"

// Virtual Memory Management 虚拟内存管理

#define VMM_READ 0x00000001
#define VMM_WRITE 0x00000002
#define VMM_EXEC 0x00000004
#define VMM_STACK 0x00000008

struct PhyPageBlock_t;
typedef struct PhyPageBlock_t PhyPageBlock_t;
struct MemMap_t;
typedef struct MemMap_t MemMap_t;

typedef struct VirtMemArea_t {
    MemMap_t *vm_mm;
    uint32_t vm_start;
    uint32_t vm_end;
    uint32_t vm_flags;
    list_ptr_t ptr;

} VirtMemArea_t;

#define lp2vma(lp, member) to_struct((lp), VirtMemArea_t, member)

typedef struct MemMap_t {
    list_ptr_t vma_list;
    VirtMemArea_t *vma_cache;
    PGD_t *pgdir;
    int vma_count;
    int ref;
    Semaphore_t sem;
    int lock_tid;
} MemMap_t;

extern PGD_t *KernelPGD;
extern PTE_t *KernelPTE;

extern uint32_t kernel_cr3;

void initKernelPageTable();

// vmm.c
int mm_ref_get(MemMap_t *mm);
void mm_ref_set(MemMap_t *mm, int val);
int mm_ref_inc(MemMap_t *mm);
int mm_ref_dec(MemMap_t *mm);
void lock_mm(MemMap_t *mm);
void unlock_mm(MemMap_t *mm);

bool isKernelCanAccess(uint32_t start, uint32_t end);
bool isUserCanAccess(uint32_t start, uint32_t end);
bool checkUserMem(MemMap_t *mm, uint32_t addr, uint32_t len, bool write);
bool copyFromUser(MemMap_t *mm, void *dst, const void *src, uint32_t len,
                  bool writable);
bool copyToUser(MemMap_t *mm, void *dst, const void *src, uint32_t len);
bool copyLegalStr(MemMap_t *mm, char *dst, const char *src, uint32_t maxn);

MemMap_t *mm_create();
void mm_destroy(MemMap_t *mm);
VirtMemArea_t *vma_create(uint32_t start, uint32_t end, uint32_t flags);
VirtMemArea_t *find_vma(MemMap_t *mm, uint32_t vaddr);
void insert_vma(MemMap_t *mm, VirtMemArea_t *vma);

int mm_map(MemMap_t *mm, uint32_t start_vaddr, uint32_t len,
               uint32_t vm_flags, VirtMemArea_t **vma_out);
int dup_mm(MemMap_t *to, MemMap_t *from);
void exit_mmap(MemMap_t *mm);

int do_pgfault(MemMap_t* mm ,InterruptFrame_t*_if,uint32_t cr2);
void test_vma();
void test_pgfault();

// page.c
void tlb_invaildate(PGD_t *pgdir, uint32_t va);
PTE_t *get_pte(PGD_t *pgdir, uint32_t va, bool create);
PhyPageBlock_t *get_page(PGD_t *pgdir, uint32_t va, PTE_t *ptep_out);

void unmap_range(PGD_t *pgdir, uint32_t start, uint32_t end);
void exit_range(PGD_t *pgdir, uint32_t start, uint32_t end);
int copy_range(PGD_t *to, PGD_t *from, uint32_t start, uint32_t end,
               bool share);
void page_remove(PGD_t *pgdir, uint32_t va);
int page_insert(PGD_t *pgdir, PhyPageBlock_t *page, uint32_t la, uint32_t perm);
PhyPageBlock_t *pgdir_alloc_page(PGD_t *pgdir, uint32_t la, uint32_t perm);
void sprint_pgdir(PGD_t *pgdir);
void sprint_cur_pgdir();
void test_pgdir();

//
void switchPageDir(uint32_t pd); // 切换页表
//
void VMM_map(PGD_t *pgd_now, uint32_t vaddr, uint32_t paddr,
             uint32_t flags); // 将物理地址映射到虚拟地址

void VMM_unmap(PGD_t *pgd_now, uint32_t vaddr); // 取消映射
uint32_t VMM_getMapping(PGD_t *pgd_now, uint32_t vaddr,
                        uint32_t *paddr); // 获取映射信息

// page_fault
void pageFaultCallback(
    InterruptFrame_t *regs); // 页中断处理


//boot_vmm
void copy_kern_pgdir(PGD_t *pgdir);

#endif