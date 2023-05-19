#include "cpu.h"
#include "debug.h"
#include "idt.h"
#include "multiboot.h"
#include "pmm.h"
#include "vmm.h"
#include "task.h"
#include "types.h"

static bool isIntrInKernel(InterruptFrame_t *_if) {
    if (_if->err_code & 0x4) {
        // printk(" In user mode.\n");
        return FALSE;
    } else {
        // printk(" In kernel mode.\n");
        return TRUE;
    }
}
void printkPageFaultErr(InterruptFrame_t *_if, uint32_t cr2) {

    printk("Page fault!\n at 0x%X, virtual address 0x%X\n", _if->eip, cr2);
    printk(" Error code: %x\n", _if->err_code);

    // bit 0 为 0 指页面不存在内存里
    if (!(_if->err_code & 0x1)) {
        printk(" the phypage wasn't present.\n");
    }
    // bit 1 为 0 表示读错误，为 1 为写错误
    if (_if->err_code & 0x2) {
        printk(" Write error.\n");
    } else {
        printk(" Read error.\n");
    }
    // bit 2 为 1 表示在用户模式打断的，为 0 是在内核模式打断的
    //  CPL = 3 时此错误位被设置
    if (_if->err_code & 0x4) {
        printk(" In user mode.\n");
    } else {
        printk(" In kernel mode.\n");
    }
    // bit 3 为 1 表示错误是由保留位覆盖造成的
    if (_if->err_code & 0x8) {
        printk(" Reserved bits being overwritten.\n");
    }
    // bit 4 为 1 表示错误发生在取指令的时候
    if (_if->err_code & 0x10) {
        printk(" The fault occurred during an instruction fetch.\n");
    }
}
void sprintkPageFaultErr(InterruptFrame_t *_if, uint32_t cr2) {

    sprintk("Page fault!\n at 0x%X, virtual address 0x%X\n", _if->eip, cr2);
    sprintk(" Error code: %x\n", _if->err_code);

    // bit 0 为 0 指页面不存在内存里
    if (!(_if->err_code & 0x1)) {
        sprintk(" the phypage wasn't present.\n");
    }
    // bit 1 为 0 表示读错误，为 1 为写错误
    if (_if->err_code & 0x2) {
        sprintk(" Write error.\n");
    } else {
        sprintk(" Read error.\n");
    }
    // bit 2 为 1 表示在用户模式打断的，为 0 是在内核模式打断的
    //  CPL = 3 时此错误位被设置
    if (_if->err_code & 0x4) {
        sprintk(" In user mode.\n");
    } else {
        sprintk(" In kernel mode.\n");
    }
    // bit 3 为 1 表示错误是由保留位覆盖造成的
    if (_if->err_code & 0x8) {
        sprintk(" Reserved bits being overwritten.\n");
    }
    // bit 4 为 1 表示错误发生在取指令的时候
    if (_if->err_code & 0x10) {
        sprintk(" The fault occurred during an instruction fetch.\n");
    }
}
int do_pgfault(MemMap_t *mm, InterruptFrame_t *_if, uint32_t cr2) {
    uint32_t error_code = _if->err_code;
    uint32_t addr = cr2;
    sprintk("\n-----do_pgfault-----\n");
    sprintkPageFaultErr(_if, cr2);
    
    sprintk("page fault信息:\n");
    if (CurrentTask != NULL) {
        sprintk("tid: %d\n", CurrentTask->tid);
    }
    sprintk("mm: 0x%08X\n", mm);
    sprintk("读写地址: 0x%08X\n", addr);
    sprintk("指令在:0x%08X\n", _if->eip);
    sprintk("此异常错误码: 0x%08X\n", error_code);

    int ret = -E_INVAL;
    VirtMemArea_t *vma = find_vma(mm, addr);
    sprintk("find_vma : 0x%08X\n", vma);
    if (vma != NULL) {
        sprintk("已通过find_vma获取此地址在 MemMap_t 中对应的 vma.\n");
        sprintk("vma->start:0x%08X vma->end:0x%08X. vmaflag:0x%X\n", vma->vm_start,
                vma->vm_end,vma->vm_flags);
    }

    // 若未找到 vma,或找到的 vma 的起始地址不正常(大于 addr),则不合法
    if (vma == NULL || vma->vm_start > addr) {
        sprintk("not valid addr %x, and  can not find it in vma\n", addr);
        goto failed;
    }

    // 检查错误码,即检查 page fault 类型
    switch (error_code & 3) {
    default:
        // 默认错误码为3 ( W/R=1, P=1)
    case 2: // 错误码: 写入缺页地址 : (W/R=1, P=0)
        if (!(vma->vm_flags & VMM_WRITE)) { // flags 校验结果异常
            sprintk(
                "do_pgfault failed: error code flag = write AND not present, "
                "but the addr's vma cannot write\n");
            goto failed;
        }
        break;
    case 1:
        // 错误码: 读取存在的地址 : (W/R=0, P=1)
        // 读取已存在页不应该出现错误码
        sprintk("do_pgfault failed: error code flag = read AND present\n");
        goto failed;
    case 0: // 错误码:读取缺页地址 (W/R=0, P=0)
        if (!(vma->vm_flags & (VMM_READ | VMM_EXEC))) { // 属性校验结果异常
            sprintk(
                "do_pgfault failed: error code flag = read AND not present, "
                "but the addr's vma cannot read or exec\n");
            goto failed;
        }
    }
    // 如果
    //  读写存在的地址
    //  或 写入不存在的地址但地址可写入
    //  或 读取不存在的地址但地址可读取
    // 则
    //  继续处理

    uint32_t perm = PTE_PAGE_USER;
    if (vma->vm_flags & VMM_WRITE) {
        perm |= PTE_PAGE_WRITEABLE;
    }
    addr = ROUNDDOWN(addr, PGSIZE);

    ret = -E_NOMEM;

    PTE_t *ptep = NULL;

    // 加载地址 addr 在所属 mm 中对应的的二级页表项,不存在则创建
    sprintk("开始恢复缺页异常: 建立对应虚拟地址的页表\n");
    if ((ptep = get_pte(mm->pgdir, addr, 1)) == NULL) {
        sprintk("get_pte in do_pgfault failed\n");
        goto failed;
    }
    sprintk("已得到此地址的页表项\n");
    // 若页表项中物理地址的值为空,则分配一个物理页并将 addr 映射过去
    if (*ptep == 0) {
        if (pgdir_alloc_page(mm->pgdir, addr, perm) == NULL) {
            sprintk("pgdir_alloc_page in do_pgfault failed\n");
            goto failed;
        }
    } else {
        PhyPageBlock_t *page = NULL;
        sprintk("do pgfault: ptep 0x%08X, pte 0x%08X\n", ptep, *ptep);
        if (*ptep & PTE_PAGE_PERSENT) {

            // COW
            // if process write to this existed readonly page (PTE_P means
            // existed), then should be here now. we can implement the delayed
            // memory space copy for fork child process
            // we didn't implement now, we will do it in future.

            //sprint_cur_pgdir();
            panic("error write a non-writable pte");
            // page = pte2page(*ptep);
        } else {
            // 若页表项中物理地址的值不为空,但 PTE_PAGE_PERSENT 不存在
            // 此页被swap
            sprintk("no swap, ptep is %x, failed\n", *ptep);
            goto failed;
        }
        // 安装页表项
        page_insert(mm->pgdir, page, addr, perm);
    }
    ret = 0;

    sprintk("\n---do_pgfault---\n");
failed:
    return ret;
}
extern MemMap_t *TestMM;
int handlePageFault(InterruptFrame_t *_if, uint32_t cr2) {
    MemMap_t *mm;
    if (TestMM != NULL) {
        mm = TestMM;
    } else {
        if (CurrentTask == NULL) {
            return -1;
        }
        mm = CurrentTask->mm;
    }
    return do_pgfault(mm, _if, cr2);
}
void pageFaultCallback(InterruptFrame_t *_if) {
    uint32_t cr2;
    asm volatile("mov %%cr2, %0" : "=r"(cr2));

    int ret;
    ret = handlePageFault(_if, cr2);

    // 页错误无法处理
    if (ret != 0) {
        printkPageFaultErr(_if, cr2);
        // 无法处理的页错误发生在进程管理初始化前
        if (CurrentTask == NULL) {
            printk("CurrentTask is NULL,System halted\n");
            haltSys();
        } else {
            // 无法处理的页错误发生在内核态
            if (isIntrInKernel(_if)) {
                printk("PageFault in kernel! System halted\n");
                haltSys();
            } else {
                // 无法处理的页错误发生在用户态
                printk("PageFault in user!\n");
                do_exit(-E_KILLED);
            }
        }
    }
}