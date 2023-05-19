#include "cpu.h"
#include "debug.h"
#include "idt.h"
#include "kmalloc.h"
#include "mm.h"
#include "pmm.h"
#include "sem.h"
#include "string.h"
#include "task.h"
#include "types.h"
#include "vmm.h"

void tlb_invaildate(PGD_t *pgdir, uint32_t va) {
    if (rcr3() == kva2pa(pgdir)) {
        invlpg(va);
    }
}
// 对于给定的vaddr,指定页目录,返回对应的 PTE.
PTE_t *get_pte(PGD_t *pgdir, uint32_t va, bool create) {
    uint32_t pgd_index = PGD_INDEX(va);
    PGD_t *pgdp = &pgdir[pgd_index];
    if (!(*pgdp & PTE_PAGE_PERSENT)) {
        if (!create) {
            return NULL;
        }
        PhyPageBlock_t *phypage;
        if ((phypage = allocPhyPages(1)) == NULL) {
            return NULL;
        }
        set_page_ref(phypage, 1);
        uint32_t pa = page2pa(phypage);
        memset(pa2kva(pa), 0, PGSIZE);
        *pgdp = pa | PTE_PAGE_USER | PTE_PAGE_WRITEABLE | PTE_PAGE_PERSENT;
    }
    return &((PTE_t *)pa2kva(PGD_ADDR(*pgdp)))[PTE_INDEX(va)];
}

// 使用pgdir获取vaddr的相关物理页PhyPageBlock_t结构
PhyPageBlock_t *get_page(PGD_t *pgdir, uint32_t va, PTE_t *ptep_out) {
    PTE_t *ptep = get_pte(pgdir, va, 0);
    if (ptep != NULL) {
        *ptep_out = ptep;
    }
    if (ptep != NULL && *ptep & PTE_PAGE_PERSENT) {
        return pte2page(*ptep);
    }
    return NULL;
}
// 移除二级页表项 ptep 对应的 page, 置零此二级页表项.
// 手动作废 pgdir 和 la 对应的 tlb
static void pte_remove_page(PGD_t *pgdir, uint32_t va, PTE_t *ptep) {
    if (*ptep & PTE_PAGE_PERSENT) {
        PhyPageBlock_t *phypage = pte2page(*ptep);
        if (page_ref_dec(phypage) <= 0) {
            freePhyPages(phypage, 1);
        }
        *ptep = 0;
        tlb_invaildate(pgdir, va);
    }
}

void unmap_range(PGD_t *pgdir, uint32_t start, uint32_t end) {
    assert((start % PGSIZE == 0 && end % PGSIZE == 0), "unmap err!");
    assert((isUserCanAccess(start, end)), "unmap err! user can access!");
    do {
        PTE_t *ptep = get_pte(pgdir, start, 0);
        if (ptep == NULL) {
            start = ROUNDDOWN(start + PGDE_MAP_NBYTE, PGDE_MAP_NBYTE);
            continue;
        }
        if (*ptep != 0) {
            pte_remove_page(pgdir, start, ptep);
        }
        start += PGSIZE;
    } while (start != 0 && start < end);
}

void exit_range(PGD_t *pgdir, uint32_t start, uint32_t end) {
    assert((start % PGSIZE == 0 && end % PGSIZE == 0), "exit_map err!");
    assert((isUserCanAccess(start, end)), "exit_map err! user can access!");

    start = ROUNDDOWN(start, PGDE_MAP_NBYTE);
    do {
        int pde_idx = PGD_INDEX(start);
        if (pgdir[pde_idx] & PTE_PAGE_PERSENT) {
            freePhyPages(pgd2page(pgdir[pde_idx]), 1);
            pgdir[pde_idx] = 0;
        }
        start += PGDE_MAP_NBYTE;
    } while (start != 0 && start < end);
}

int copy_range(PGD_t *to, PGD_t *from, uint32_t start, uint32_t end,
               bool share) {
    assert((start % PGSIZE == 0 && end % PGSIZE == 0), "copy_range err!");
    assert(isUserCanAccess(start, end), "copy_range err! user can access!");
    // copy content by page unit.
    do {

        PTE_t *ptep = get_pte(from, start, 0), *nptep;
        if (ptep == NULL) {
            start = ROUNDDOWN(start + PGDE_MAP_NBYTE, PGDE_MAP_NBYTE);
            continue;
        }

        if (*ptep & PTE_PAGE_PERSENT) {
            if ((nptep = get_pte(to, start, 1)) == NULL) {
                return -E_NOMEM;
            }
            uint32_t perm = (*ptep & PTE_USER);

            PhyPageBlock_t *page = pte2page(*ptep);

            PhyPageBlock_t *npage = allocPhyPages(1);
            assert(page != NULL, "copy_range err!");
            assert(npage != NULL, "copy_range err!");
            int ret = 0;

            void *kva_src = page2kva(page);
            void *kva_dst = page2kva(npage);

            memcpy(kva_dst, kva_src, PGSIZE);

            ret = page_insert(to, npage, start, perm);
            assert(ret == 0, "copy_range err!");
        }
        start += PGSIZE;
    } while (start != 0 && start < end);
    return 0;
}
// 移除 pgdir 中 la 对应的二级页表项
void page_remove(PGD_t *pgdir, uint32_t va) {
    PTE_t *ptep = get_pte(pgdir, va, 0);
    if (ptep != NULL) {
        pte_remove_page(pgdir, va, ptep);
    }
}
/**
 * 建立 pte=>phypage 的映射.
 * 从虚拟地址到 va 到 page 在一级页表pgdir下的映射.
 * 1. 获取 va 对应的 pte
 * 2. 如果 pte 已存在,就移除掉
 * 3. 将 pte 指向的 page 指定为参数中的 page,以及一些权限.
 * 4. 更新 TLB
 */
int page_insert(PGD_t *pgdir, PhyPageBlock_t *page, uint32_t va,
                uint32_t perm) {
    // 从pgdir 中获取 va 对应的 pte，若没有则新建 pte 项
    PTE_t *ptep = get_pte(pgdir, va, 1);
    // printk("pi1");
    if (ptep == NULL) {
        return -E_NOMEM;
    }
    page_ref_inc(page);
    // 如果 pte 已存在,就移除掉
    if (*ptep & PTE_PAGE_PERSENT) {
        // 计算 pte 条目映射的 page
        PhyPageBlock_t *p = pte2page(*ptep);
        // va 映射的 page 与新 page 是否相等
        // = 插入的页是否是新页
        if (p == page) {
            // 插入的页和原来一样，旧页前面加过的减回来
            page_ref_dec(page);
        } else {
            // 插入新页，抛弃旧页
            pte_remove_page(pgdir, va, ptep);
            // printk("pi3");
        }
    }
    sprintk(" insert page :0x%08X\n", page2pa(page));
    *ptep = page2pa(page) | PTE_PAGE_PERSENT | perm;
    // printk("pi4");
    tlb_invaildate(pgdir, va);
    return 0;
}
/**
 *
 * 新建一个在 pgdir 下, 从未映射过的虚拟地址 la 到物理空间的的映射
 * 1. la 是从未映射过的,所以肯定要分配一个物理空间 page 与其对应.
 * 2. 建立页表
 * 3. 将新 page 设置为可交换的.
 *
 * la: liner address,线性地址
 * perm: permission,权限
 */
PhyPageBlock_t *pgdir_alloc_page(PGD_t *pgdir, uint32_t la, uint32_t perm) {
    PhyPageBlock_t *page = allocPhyPages(1);
    if (page != NULL) {
        if (page_insert(pgdir, page, la, perm) != 0) {
            freePhyPages(page, 1);
            return NULL;
        }
    }

    return page;
}

static void ________________________SPACE__________________________();

static const char *perm2str(int perm) {
    static char str[4];
    str[0] = (perm & PTE_PAGE_USER) ? 'u' : '-';
    str[1] = 'r';
    str[2] = (perm & PTE_PAGE_WRITEABLE) ? 'w' : '-';
    str[3] = '\0';
    return str;
}

void sprint_pgdir(PGD_t *pgdir) {
    sprintk("\n--- 页表信息 begin ---\n");
    for (int i = 0; i < PGD_ECOUNT; i++) {
        if (pgdir[i] != 0) {
            sprintk("pgd[%d]:0x%08X,%s\n", i, pgdir[i], perm2str(pgdir[i]));
            PTE_t *ptep = PGD_ADDR(pgdir[i]) + KERNEL_BASE;
            for (int i = 0; i < 5; i++) {
                if (ptep[i] != 0) {
                    sprintk(" pte[%d]:in0x%08X,%s,va:0x%08X-0x%08X\n", i, ptep,
                            perm2str(ptep[i]), ptep[i], ptep[i] + PGSIZE - 1);
                }
            }
        }
    }
    sprintk("\n--- 页表信息 end ---\n\n");
}
void sprint_cur_pgdir() {
    PGD_t *pgdir = CurrentTask->cr3 + KERNEL_BASE;
    sprintk("\n--- 页表信息 begin ---\n");
    for (int i = 0; i < PGD_ECOUNT; i++) {
        if (pgdir[i] != 0) {
            sprintk("pgd[%d]:0x%08X,%s\n", i, pgdir[i], perm2str(pgdir[i]));
            PTE_t *ptep = PGD_ADDR(pgdir[i]) + KERNEL_BASE;
            for (int i = 0; i < 5; i++) {
                if (ptep[i] != 0) {
                    sprintk(" pte[%d]:in0x%08X,%s,va:0x%08X-0x%08X\n", i, ptep,
                            perm2str(ptep[i]), ptep[i], ptep[i] + PGSIZE - 1);
                }
            }
        }
    }
    sprintk("\n--- 页表信息 end ---\n\n");
}
void test_pgdir(void) {
    sprintk("\nTEST VMM PGDIR:\n");
    int s = 1;
    PGD_t *pgdir = kmalloc(PGSIZE);
    assert(pgdir != NULL && (uint32_t)PG_OFFSET(pgdir) == 0, "1");
    sprintk("%d ", s++);
    assert(get_page(pgdir, 0x0, NULL) == NULL, "2");
    sprintk("%d ", s++);

    PhyPageBlock_t *p1 = allocPhyPages(1);
    PhyPageBlock_t *p2 = allocPhyPages(1);
    PhyPageBlock_t *p3 = allocPhyPages(1);

    sprintk("p1:0x%08X\n", page2pa(p1));
    sprintk("\ninsert p1-v0x0\n");
    assert(page_insert(pgdir, p1, 0x0, 0) == 0, "3");
    // p1-v0x0 p1ref+1
    sprintk("%d ", s++);

    PTE_t *ptep;
    assert((ptep = get_pte(pgdir, 0x0, 0)) != NULL, "4");
    sprintk("%d ", s++);
    assert(pte2page(*ptep) == p1, "5");
    sprintk("%d ", s++);
    sprintk("\nref p1:%d\n", page_ref(p1));
    assert(page_ref(p1) == 1, "6");
    sprintk("%d ", s++);

    ptep = &((PTE_t *)pa2kva(PGD_ADDR(pgdir[0])))[1];
    assert(get_pte(pgdir, PGSIZE, 0) == ptep, "7");
    sprintk("%d ", s++);

    // for (int i = 0; i < 16; i++) {
    //     p3 = (PhyPageBlock_t *)allocPhyPages(1);
    //     sprintk(" p3:0x%08X\n", page2pa(p3));
    // }
    sprintk("\n");

    // for (int i = 0; i < 16; i++) {
    //     p3 = (PhyPageBlock_t *)allocPhyPages(1);
    //     sprintk(" p3:0x%08X\n", page2pa(p3));
    // }
    // p2-v4K p2ref+1
    sprintk("\ninsert p2-v4K\n");
    sprintk("p2:0x%08X\n", page2pa(p2));
    assert(page_insert(pgdir, p2, PGSIZE, PTE_PAGE_USER | PTE_PAGE_WRITEABLE) ==
               0,
           "8");
    sprintk("%d ", s++);
    assert((ptep = get_pte(pgdir, PGSIZE, 0)) != NULL, "9");
    sprintk("%d ", s++);
    assert(*ptep & PTE_PAGE_USER, "10");
    sprintk("%d ", s++);
    assert(*ptep & PTE_PAGE_WRITEABLE, "11");
    sprintk("%d ", s++);
    assert(pgdir[0] & PTE_PAGE_USER, "12");
    sprintk("%d ", s++);
    sprintk("\nref p1:%d\n", page_ref(p1));
    sprintk("\nref p2:%d\n", page_ref(p2));
    assert(page_ref(p2) == 1, "13");
    sprintk("%d ", s++);

    // p1-v4K p1ref+1
    sprintk("\ninsert p1-v4K\n");
    sprintk("p1:0x%08X\n", page2pa(p1));
    assert(page_insert(pgdir, p1, PGSIZE, 0) == 0, "14");
    sprintk("%d ", s++);
    sprintk("\nref p1:%d\n", page_ref(p1));
    sprintk("\nref p2:%d\n", page_ref(p2));
    assert(page_ref(p1) == 2, "15");
    sprintk("%d ", s++);
    assert(page_ref(p2) == 0, "16");
    sprintk("%d ", s++);
    assert((ptep = get_pte(pgdir, PGSIZE, 0)) != NULL, "17");
    sprintk("%d ", s++);
    assert(pte2page(*ptep) == p1, "18");
    sprintk("%d ", s++);
    assert((*ptep & PTE_PAGE_USER) == 0, "19");
    sprintk("%d ", s++);

    sprintk("\ninsert p3\n");

    // for (int i = 0; i < 16; i++) {
    //     p3 = (PhyPageBlock_t *)allocPhyPages(1);
    //     sprintk(" p3:0x%08X\n", page2pa(p3));
    // }

    page_insert(pgdir, p3, PGSIZE * 3, PTE_PAGE_USER);
    sprintk("\nref p1:%d\n", page_ref(p1));
    sprintk("\nref p2:%d\n", page_ref(p2));
    sprintk("\nref p3:%d\n", page_ref(p3));

    sprint_pgdir(pgdir);

    // remove
    page_remove(pgdir, 0x0);
    sprintk("\nremove v0x0\n");
    sprintk("\nref p1:%d\n", page_ref(p1));
    sprintk("\nref p2:%d\n", page_ref(p2));
    assert(page_ref(p1) == 1, "20");
    sprintk("%d ", s++);
    assert(page_ref(p2) == 0, "21");
    sprintk("%d ", s++);

    page_remove(pgdir, PGSIZE);
    assert(page_ref(p1) == 0, "22");
    sprintk("%d ", s++);
    assert(page_ref(p2) == 0, "23");
    sprintk("%d ", s++);

    assert(page_ref(pgd2page(pgdir[0])) == 1, "24");
    sprintk("%d ", s++);
    freePhyPages(pgd2page(pgdir[0]), 1);
    pgdir[0] = 0;

    // print_pgdir2(pgdir);

    sprintk("\nPGDIR TEST OK!!!\n");
}

static void ________________________SPACE__________________________();
void switchPageDir(uint32_t pd) { lcr3(pd); }

void VMM_map(PGD_t *pgd_now, uint32_t vaddr, uint32_t paddr, uint32_t flags) {
    uint32_t pgd_index = PGD_INDEX(vaddr);
    uint32_t pte_index = PTE_INDEX(vaddr);
    PTE_t *pte = (PTE_t *)(pgd_now[pgd_index] & PAGE_MASK);

    if (pte == 0) {
        pte = (PTE_t *)kmalloc(PGSIZE);
        pgd_now[pgd_index] =
            (uint32_t)pte | PTE_PAGE_PERSENT | PTE_PAGE_WRITEABLE;
        bzero(pte, PGSIZE);
    } else {
        pte = (PTE_t *)((uint32_t)pte + KERNEL_BASE);
    }
    pte[pte_index] = (paddr & PAGE_MASK) | flags; // 映射物理地址到PTE中
    // 刷新页表缓存(TLB)
    invlpg(vaddr);
}

void VMM_unmap(PGD_t *pgd_now, uint32_t vaddr) {
    uint32_t pgd_index = PGD_INDEX(vaddr);
    uint32_t pte_index = PTE_INDEX(vaddr);
    PTE_t *pte = (PTE_t *)(pgd_now[pgd_index] & PAGE_MASK);

    if (pte == 0) {
        return;
    }

    pte = (PTE_t *)((uint32_t)pte + KERNEL_BASE);
    pte[pte_index] = 0;

    invlpg(vaddr);
}
uint32_t VMM_getMapping(PGD_t *pgd_now, uint32_t vaddr, uint32_t *paddr) {
    uint32_t pgd_index = PGD_INDEX(vaddr);
    uint32_t pte_index = PTE_INDEX(vaddr);
    PTE_t *pte = (PTE_t *)(pgd_now[pgd_index] & PAGE_MASK);

    if (pte == 0) {
        return 0;
    }
    pte = (PTE_t *)((uint32_t)pte + KERNEL_BASE);
    if (pte[pte_index] != 0 && paddr != 0) {
        *paddr = pte[pte_index] & PAGE_MASK;
        return 1;
    }
    return 0;
}
