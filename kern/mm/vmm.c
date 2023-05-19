#include "vmm.h"
#include "debug.h"
#include "idt.h"
#include "kmalloc.h"
#include "mm.h"
#include "pmm.h"
#include "sem.h"
#include "string.h"

#include "task.h"
#include "types.h"

// TOOL FUNC
int mm_ref_get(MemMap_t *mm) { return mm->ref; }
void mm_ref_set(MemMap_t *mm, int val) { mm->ref = val; }
int mm_ref_inc(MemMap_t *mm) {
    mm->ref++;
    return mm->ref;
}
int mm_ref_dec(MemMap_t *mm) {
    mm->ref--;
    return mm->ref;
}
void lock_mm(MemMap_t *mm) {
    if (mm != NULL) {
        acquireSem(&(mm->sem));
        if (CurrentTask != NULL) {
            mm->lock_tid = CurrentTask->tid;
        }
    }
}
void unlock_mm(MemMap_t *mm) {
    if (mm != NULL) {
        releaseSem(&(mm->sem));
        mm->lock_tid = 0;
    }
}

bool isKernelCanAccess(uint32_t start, uint32_t end) {
    return (KERNEL_BASE <= (start) && (start) < (end) && (end) <= KERNEL_TOP);
}
bool isUserCanAccess(uint32_t start, uint32_t end) {
    return (USER_BASE <= start && start < end && end <= USER_TOP);
}

//  按给定的mm,地址区间和读写属性,检查地址是否合法.
//  防止用户进程向内核传递指向内核空间的指针。
//
//  此函数有两种情况:
//  1.内核进程调用,此时 mm 为 NULL,则检查地址是否处于内核空间;
//  2.内核态的用户进程调用,此时 mm 不为NULL,则检查是否处于用户区间,且是否符合 mm
//  属性.读0 写1
bool checkUserMem(MemMap_t *mm, uint32_t addr, uint32_t len, bool write) {
    if (mm != NULL) {
        if (!isUserCanAccess(addr, addr + len)) {
            return FALSE;
        }
        VirtMemArea_t *vma;
        uint32_t start = addr;
        uint32_t end = addr + len;
        while (start < len) {
            vma = find_vma(mm, start);
            if (vma == NULL || start < vma->vm_start) {
                return FALSE;
            }
            if (!(vma->vm_flags & (write ? VMM_WRITE : VMM_READ))) {
                return 0;
            }
            if (write && (vma->vm_flags & VMM_STACK)) {
                if (start < vma->vm_start + PGSIZE) {
                    return 0;
                }
            }
            start = vma->vm_end;
        }
        return TRUE;
    }
    return isKernelCanAccess(addr, addr + len);
}
bool copyFromUser(MemMap_t *mm, void *dst, const void *src, uint32_t len,
                  bool writable) {
    if (!checkUserMem(mm, (uint32_t)src, len, writable)) {
        return 0;
    }
    memcpy(dst, src, len);
    return 1;
}
bool copyToUser(MemMap_t *mm, void *dst, const void *src, uint32_t len) {
    if (!checkUserMem(mm, (uint32_t)dst, len, 1)) {
        return 0;
    }
    memcpy(dst, src, len);
    return 1;
}
// 带合法性检测的字符串复制函数
// 从src到 src+maxn 必须可读
bool copyLegalStr(MemMap_t *mm, char *dst, const char *src, uint32_t maxn) {
    uint32_t alen;
    uint32_t part = ROUNDDOWN((uint32_t)src + PGSIZE, PGSIZE) - (uint32_t)src;
    while (1) {
        if (part > maxn) {
            part = maxn;
        }
        if (!checkUserMem(mm, (uint32_t)src, part, 0)) {
            return 0;
        }
        if ((alen = strnlen(src, part)) < part) {
            memcpy(dst, src, alen + 1);
            return 1;
        }
        if (part == maxn) {
            return 0;
        }
        memcpy(dst, src, part);
        dst += part;
        src += part;
        maxn -= part;
        part = PGSIZE;
    }
}
static void ______________________SPACE______________________();

MemMap_t *mm_create() {
    MemMap_t *mm = kmalloc(sizeof(MemMap_t));

    if (mm != NULL) {
        initList(&(mm->vma_list));
        mm->vma_cache = NULL;
        mm->pgdir = NULL;
        mm->vma_count = 0;
        mm_ref_set(mm, 0);
        initSem(&(mm->sem), 1, "MM sem");
    }
    return mm;
}
void mm_destroy(MemMap_t *mm) {
    if(mm == NULL){
        return;
    }
    if (mm_ref_get(mm) <= 0) {
        list_ptr_t *list = &(mm->vma_list);
        list_ptr_t *i;
        while ((i = listGetNext(list)) != list) {
            listDel(i);
            kfree(lp2vma(i, ptr), sizeof(VirtMemArea_t));
        }
        kfree(mm, sizeof(MemMap_t));
        mm = NULL;
    } else {
        mm_ref_dec(mm);
    }
}

VirtMemArea_t *vma_create(uint32_t start, uint32_t end, uint32_t flags) {
    VirtMemArea_t *vma = kmalloc(sizeof(VirtMemArea_t));

    if (vma != NULL) {
        vma->vm_start = start;
        vma->vm_end = end;
        vma->vm_flags = flags;
    }
    return vma;
}
// 从mm中查找vaddr对应的vma
VirtMemArea_t *find_vma(MemMap_t *mm, uint32_t vaddr) {
    VirtMemArea_t *vma = NULL;
    if (mm != NULL) {
        vma = mm->vma_cache;
        if (!(vma != NULL && vma->vm_start <= vaddr && vma->vm_end > vaddr)) {
            bool found = 0;
            list_ptr_t *i;
            list_ptr_t *list = &mm->vma_list;
            listForEach(i, list) {
                vma = lp2vma(i, ptr);
                if (vma->vm_start <= vaddr && vma->vm_end > vaddr) {
                    found = TRUE;
                    break;
                }
            }
            if (!found) {
                vma = NULL;
            }
        }
        if (vma != NULL) {
            mm->vma_cache = vma;
        }
    }
    return vma;
}
static void check_vma_overlap(VirtMemArea_t *prev, VirtMemArea_t *next) {
    if (prev->vm_start >= prev->vm_end) {
        sprintk("\nPanic: Fatal error\n");
        sprintk("prev_start>=prev_end:0x%08X,0x%08X\n", prev->vm_start,
                prev->vm_end);
        panic(" vma overlap :pstart>=pend!");
    }
    if (prev->vm_end > next->vm_start) {
        sprintk("\nPanic: Fatal error\n");
        sprintk("prev_end>=next_start:0x%08X,0x%08X\n", prev->vm_end,
                next->vm_start);
        panic(" vma overlap :pend>=nstart!");
    }
    if (next->vm_start >= next->vm_end) {
        sprintk("\nPanic: Fatal error\n");
        sprintk("next_start>=next_end:0x%08X,0x%08X\n", next->vm_start,
                next->vm_end);
        panic(" vma overlap :nstart>=pend!");
    }
}
void insert_vma(MemMap_t *mm, VirtMemArea_t *vma) {
    if (vma->vm_start >= vma->vm_end) {
        panic(" vma overlap :start>=end!");
    }

    list_ptr_t *list = &(mm->vma_list);
    list_ptr_t *lp_prev = list;
    list_ptr_t *lp_next;

    list_ptr_t *i = NULL;
    listForEach(i, list) {
        VirtMemArea_t *vma_prev = lp2vma(i, ptr);
        if (vma_prev->vm_start > vma->vm_start) {
            break;
        }
        lp_prev = i;
    }
    lp_next = listGetNext(lp_prev);

    if (lp_prev != list) {
        check_vma_overlap(lp2vma(lp_prev, ptr), vma);
    }
    if (lp_next != list) {
        check_vma_overlap(vma, lp2vma(lp_next, ptr));
    }

    vma->vm_mm = mm;
    listAddAfter(lp_prev, &(vma->ptr));
    mm->vma_count++;
}

int mm_map(MemMap_t *mm, uint32_t start_vaddr, uint32_t len,
               uint32_t vm_flags, VirtMemArea_t **vma_out) {
    uint32_t start = ROUNDDOWN(start_vaddr, PGSIZE);
    uint32_t end = ROUNDUP(start_vaddr + len, PGSIZE);
    if (!isUserCanAccess(start, end)) {
        return -E_INVAL;
    }
    if (mm == NULL) {
        return -E_INVAL;
    }
    int ret = -E_INVAL;

    VirtMemArea_t *vma;
    vma = find_vma(mm, start);
    if (vma != NULL && end > vma->vm_start) {
        return -E_INVAL;
    }
    vma = vma_create(start, end, vm_flags);
    if (vma == NULL) {
        return -E_NOMEM;
    }
    insert_vma(mm, vma);
    if (vma_out != NULL) {
        *vma_out = vma;
    }
    return E_OK;
}
int dup_mm(MemMap_t *to, MemMap_t *from) {
    if (to == NULL || from == NULL) {
        return -E_PNULL;
    }
    list_ptr_t *list = &from->vma_list;
    list_ptr_t *i = list;
    while ((i = listGetPrev(i)) != list) {
        VirtMemArea_t *pvma = lp2vma(i, ptr);
        VirtMemArea_t *nvma =
            vma_create(pvma->vm_start, pvma->vm_end, pvma->vm_flags);
        if (nvma == NULL) {
            return -E_NOMEM;
        }
        insert_vma(to, nvma);

        bool share = FALSE;
        if (copy_range(to->pgdir, from->pgdir, pvma->vm_start, pvma->vm_end,
                       share) != 0) {
            return -E_NOMEM;
        }
    }
    return 0;
}
void exit_mmap(MemMap_t *mm) {
    if (mm == NULL || mm_ref_get(mm) != 0) {
        panic("exit mm err!");
    }
    PGD_t *pgdir = mm->pgdir;

    list_ptr_t *list = &(mm->vma_list);
    list_ptr_t *i = list;

    listForEach(i, list) {
        VirtMemArea_t *vma = lp2vma(i, ptr);
        unmap_range(pgdir, vma->vm_start, vma->vm_end);
    }
    listForEach(i, list) {
        VirtMemArea_t *vma = lp2vma(i, ptr);
        exit_range(pgdir, vma->vm_start, vma->vm_end);
    }
}

static void ______________________SPACE______________________();


void test_vma() {

    sprintk("----开始测试 vma结构.----\n");
    sprintk("   测试点: 是否正确把 vma插入到 mm,是否有重叠,是否能从 mm "
            "找到某个地址所在的 vma.\n");

    MemMap_t *mm = mm_create();
    assert(mm != NULL, "0");

    int step1 = 10, step2 = step1 * 10;
    sprintk("   从 5 到 50, 以及从 55 到 500,每隔 5 个字节创建一个 vma, 长度是 "
            "2;全部插入到 mm 链表中.\n");
    int i;
    for (i = step1; i >= 1; i--) {
        VirtMemArea_t *vma = vma_create(i * 5, i * 5 + 2, 0);
        assert(vma != NULL, "0");
        insert_vma(mm, vma);
    }

    for (i = step1 + 1; i <= step2; i++) {
        VirtMemArea_t *vma = vma_create(i * 5, i * 5 + 2, 0);
        assert(vma != NULL, "0");
        insert_vma(mm, vma);
    }
    sprintk("   插入结束,mm 所维护的 vma 数量为%d\n", mm->vma_count);

    list_ptr_t *le = listGetNext(&(mm->vma_list));

    for (i = 1; i <= step2; i++) {
        assert(le != &(mm->vma_list), "0");
        VirtMemArea_t *mmap = lp2vma(le, ptr);
        assert(mmap->vm_start == i * 5 && mmap->vm_end == i * 5 + 2, "0");
        le= listGetNext(le);
    }

    for (i = 5; i <= 5 * step2; i += 5) {
        VirtMemArea_t *vma1 = find_vma(mm, i);
        assert(vma1 != NULL, "0");
        VirtMemArea_t *vma2 = find_vma(mm, i + 1);
        assert(vma2 != NULL, "0");
        VirtMemArea_t *vma3 = find_vma(mm, i + 2);
        assert(vma3 == NULL, "0");
        VirtMemArea_t *vma4 = find_vma(mm, i + 3);
        assert(vma4 == NULL, "0");
        VirtMemArea_t *vma5 = find_vma(mm, i + 4);
        assert(vma5 == NULL, "0");

        assert(vma1->vm_start == i && vma1->vm_end == i + 2, "0");
        assert(vma2->vm_start == i && vma2->vm_end == i + 2, "0");
    }

    for (i = 4; i >= 0; i--) {
        VirtMemArea_t *vma_below_5 = find_vma(mm, i);
        if (vma_below_5 != NULL) {
            sprintk("vma_below_5: i %x, start %x, end %x\n", i,
                    vma_below_5->vm_start, vma_below_5->vm_end);
        }
        assert(vma_below_5 == NULL, "0");
    }

    mm_destroy(mm);

    //  assert(nr_free_pages_store == nr_free_pages());

    sprintk("---check vma succeeded!---\n");
}

MemMap_t *TestMM;

// 内核页表和页目录
extern PGD_t *KernelPGD;
extern PTE_t *KernelPTE;
void test_pgfault(){
    sprintk("test page fault\n");

    TestMM = mm_create();
    MemMap_t* mm =  TestMM;
    
    
    assert(mm != NULL,"0");

    PGD_t *boot_pgdir = KernelPGD;
    //bzero(boot_pgdir,PGSIZE);
    //copy_kern_pgdir(boot_pgdir);
    //sprint_pgdir(boot_pgdir);

    //sprintk("   切换页表:0x%08X\n",boot_pgdir);
    // switchPageDir(boot_pgdir - KERNEL_BASE);
    //sprintk("   切换页表成功:\n");

    PGD_t *pgdir = mm->pgdir = boot_pgdir;
    sprintk("此mm_struct的一级页表地址为: 0x%08lx.\n", pgdir);
    assert(pgdir[0] == 0,"0");

    sprintk("创建一个页目录项对应大小的 vma(1024*4KB=4MB), "
        "物理地址区间为[0,4M), 可写入,插入到 mm 中.\n");
    VirtMemArea_t *vma = vma_create(0, PGDE_MAP_NBYTE, VMM_WRITE);
    assert(vma != NULL,"0");

    insert_vma(mm, vma);

    uint32_t addr = 0x100; // = 256B
    assert(find_vma(mm, addr) == vma,"0");

    sprintk(" 开始对此区域进行写入.\n");
    int i, sum = 0;
    for (i = 0; i < 100; i++) {
        //sprintk("第%d次写入\n",i+1);
        *(char *)(addr + i) = i;
        sum += i;
    }
    for (i = 0; i < 100; i++) {
        //sprintk("第%d次读取\n",i+1);
        sum -= *(char *)(addr + i);
    }
    assert(sum == 0,"0");

    // sprint_pgdir(boot_pgdir);

    sprintk("读写完完成,移除页表项\n");
    page_remove(pgdir, ROUNDDOWN(addr, PGSIZE));
    sprintk("已移除页表项\n");

    freePhyPages(pgd2page(pgdir[0]),1);
    pgdir[0] = 0;

    mm->pgdir = NULL;
    mm_destroy(mm);
    mm = NULL;


    sprintk("测试通过: page fault");
}