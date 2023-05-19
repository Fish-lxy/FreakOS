#include "task.h"
#include "cpu.h"
#include "debug.h"
#include "elf.h"
#include "file.h"
#include "gdt.h"
#include "idt.h"
#include "intr_sync.h"
#include "kmalloc.h"
#include "list.h"
#include "mm.h"
#include "pmm.h"
#include "sched.h"
#include "string.h"
#include "syscall.h"
#include "sysfile.h"
#include "types.h"
#include "vmm.h"

extern int init_task_func(void *arg);
extern int test_task(void *arg);

void initTask();
static Task_t *allocTask();
static char *setTaskName(Task_t *task, const char *name);
static char *getTaskName(Task_t *task);
static uint32_t get_new_tid();
static int32_t setupNewKstack(Task_t *task);
static void freeKstack(Task_t *task);

static Task_t *find_task(int tid);

void kernel_thread_entry_s();
void fork_return_s(InterruptFrame_t *_if);
void switch_to_s(Context_t *from, Context_t *to);

int32_t createKernelThread(int (*func)(void *), void *arg,
                           uint32_t clone_flags);
int32_t do_fork(uint32_t clone_flags, uint32_t stack, InterruptFrame_t *_if);
int32_t copy_mm(uint32_t clone_flags, Task_t *task);
static int copy_files(uint32_t clone_flags, Task_t *task);
static void put_files(Task_t *task);

int32_t copy_task(Task_t *task, uint32_t esp, InterruptFrame_t *_if);
int do_exit(int err_code);
static void forkret(void);

Task_t *CurrentTask;
Task_t *idle_task;
Task_t *init_task;

TaskList_t TaskList; // 任务链表首节点
uint32_t TaskCount = 0;

void initTask() {

    initRunQueue();

    initList(&(TaskList.ptr));
    // 当前 main 函数的执行流即为系统首个内核进程,
    // 在完成各个子系统初始化后变为系统空闲进程 idle
    // 剩下的初始化工作交给第二个进程 init

    // 构造首个进程 idle
    idle_task = allocTask();
    listAdd(&TaskList.ptr, &(idle_task->ptr));
    if (idle_task == NULL) {
        panic("Can not create idle task!");
    }

    idle_task->tid = 0;
    idle_task->state = TASK_RUNNABLE;
    idle_task->wait_state = WT_NOTWAIT;
    idle_task->kstack = KernelStack;
    idle_task->need_resched = 1;

    idle_task->files = files_create();
    if (idle_task->files == NULL) {
        panic("idle task files create error!");
    }
    files_ref_inc(idle_task->files);

    setTaskName(idle_task, "idle");

    TaskCount++;
    CurrentTask = idle_task;

    int init_tid = createKernelThread(init_task_func, "Hello, world!", 0);
    init_task = find_task(init_tid);

    if (init_tid < 0) {
        panic("create init process failed.");
    }
}
Task_t *allocTask() {
    Task_t *task = (Task_t *)kmalloc(sizeof(Task_t));

    if (task == NULL) {
        return NULL;
    }
    if (task != NULL) {
        task->state = TASK_UNINIT;
        task->wait_state = WT_NOTWAIT;
        task->time_slice = 0;
        task->need_resched = 0;
        task->tid = -1;
        task->runs = 0;
        task->kstack = 0;
        task->parent = NULL;
        task->cptr = task->optr = task->yptr = NULL;
        task->mm = NULL;
        memset(&(task->context), 0, sizeof(Context_t));
        task->_if = NULL;
        task->cr3 = kernel_cr3;
        task->flags = 0;
        memset(task->name, 0, TASK_NAME_LEN);

        task->files = NULL;

        return task;
    }
}
// 申请内核栈，TSS段将使用
static int32_t setupNewKstack(Task_t *task) {
    uint32_t stack = (uint32_t)kmalloc(KSTACKSIZE);
    if (stack != NULL) {
        task->kstack = (uint32_t *)stack;
        return 0;
    }
    return E_NOMEM;
}
static void freeKstack(Task_t *task) { kfree(task->kstack, KSTACKSIZE); }

char *setTaskName(Task_t *task, const char *name) {
    memset(task->name, 0, sizeof(task->name));
    return memcpy(task->name, name, TASK_NAME_LEN);
}
char *getTaskName(Task_t *task) {
    static char name[TASK_NAME_LEN + 1];
    memset(name, 0, sizeof(name));
    return memcpy(name, task->name, TASK_NAME_LEN);
}
static void add_task(Task_t *task) {
    listAdd(&TaskList.ptr, &(task->ptr));
    task->yptr = NULL;
    if ((task->optr = task->parent->cptr) != NULL) {
        task->optr->yptr = task;
    }
    task->parent->cptr = task;
    TaskCount++;
}
static void del_task(Task_t *task) {
    listDel(&(task->ptr));
    if (task->optr != NULL) {
        task->optr->yptr = task->yptr;
    }
    if (task->yptr != NULL) {
        task->yptr->optr = task->optr;
    } else {
        task->parent->cptr = task->optr;
    }
    TaskCount--;
}
static Task_t *find_task(int tid) {
    list_ptr_t *i;
    list_ptr_t *list = &TaskList.ptr;
    listForEach(i, list) {
        Task_t *task = lp2task(i, ptr);
        if (tid == task->tid) {
            return task;
        }
    }
    return NULL;
}

uint32_t get_new_tid() {
    static uint32_t newestPid = 0;

    newestPid = TaskCount - 1;
    newestPid++;
    return newestPid;
}
static void forkret(void) { fork_return_s(CurrentTask->_if); }

// 建立一个执行函数func的内核线程
int32_t createKernelThread(int (*func)(void *), void *arg,
                           uint32_t clone_flags) {
    // 保存内核线程的临时中断帧

    InterruptFrame_t interruptFrame;

    memset(&interruptFrame, 0, sizeof(InterruptFrame_t));
    interruptFrame.cs = KERNEL_CS;
    interruptFrame.ds = interruptFrame.es = interruptFrame.ss = KERNEL_DS;
    interruptFrame.ebx = (uint32_t)func;
    interruptFrame.edx = (uint32_t)arg;
    interruptFrame.eip = (uint32_t)kernel_thread_entry_s;

    return do_fork(clone_flags | CLONE_VM, 0, &interruptFrame);
}
// 创建一个子进程所需的数据结构
int32_t do_fork(uint32_t clone_flags, uint32_t stack, InterruptFrame_t *_if) {
    // printk("Stage11_fork\n");
    Task_t *task;
    if (TaskCount >= MAX_TASKS) {
        return -E_NO_FREE_TASK;
    }
    task = allocTask();
    if (task == NULL) {
        return -E_NOMEM;
    }

    int ret;
    task->parent = CurrentTask;
    if (setupNewKstack(task) != 0) {
        ret = -E_NOMEM;
        goto failed_and_clean_task;
    }
    if (copy_files(clone_flags, task) != 0) {
        ret = -E_NOMEM;
        goto failed_and_clean_kstack;
    }
    if (copy_mm(clone_flags, task) != 0) {
        ret = -E_NOMEM;
        goto failed_and_clean_files;
    }
    copy_task(task, stack, _if);

    bool flag;
    intr_save(flag);
    {
        task->tid = get_new_tid();
        ret = task->tid;
        add_task(task);
    }
    intr_restore(flag);

    sprintk("新进程%d建立\n", ret);
    wakeupTask(task);

    // 子进程不会执行至此
    ret = task->tid; // 向父进程返回新的子进程的 pid

out:
    return ret;
failed_and_clean_files:
    put_files(task);
failed_and_clean_kstack:
    kfree(task->kstack, KSTACKSIZE);
failed_and_clean_task:
    kfree(task, sizeof(Task_t));
failed_out:
    return ret;
}
extern PGD_t *KernelPGD;
static int setup_pgdir(MemMap_t *mm) {
    PhyPageBlock_t *phypage;
    if ((phypage = allocPhyPages(1)) == NULL) {
        return -E_NOMEM;
    }
    PGD_t *pgdir = page2kva(phypage);
    memcpy(pgdir, KernelPGD, PGSIZE);
    mm->pgdir = pgdir;
    return 0;
}
static int put_pgdir(MemMap_t *mm) { freePhyPages(kva2page(mm->pgdir), 1); }
// 根据clone_flag标志复制或共享进程内存管理结构
int32_t copy_mm(uint32_t clone_flags, Task_t *task) {

    MemMap_t *mm;
    MemMap_t *old_mm = CurrentTask->mm;

    if (old_mm == NULL) {
        return 0;
    }
    if (clone_flags & CLONE_VM) {
        mm = old_mm;
        goto out;
    }
    int ret = -E_NOMEM;
    if ((mm = mm_create()) == NULL) {
        goto failed_mm;
    }
    if (setup_pgdir(mm) != 0) {
        goto failed_pgdir;
    }
    lock_mm(old_mm);
    ret = dup_mm(mm, old_mm);
    unlock_mm(old_mm);

    if (ret != 0) {
        goto failed_dup_mm;
    }

out:
    mm_ref_inc(mm);
    task->mm = mm;
    task->cr3 = kva2pa(mm->pgdir);
    return 0;

failed_dup_mm:
    exit_mmap(mm);
    put_pgdir(mm);
failed_pgdir:
    mm_destroy(mm);
failed_mm:
    return ret;
}
static int copy_files(uint32_t clone_flags, Task_t *task) {
    FileStruct_t *files;
    FileStruct_t *old_files = CurrentTask->files;

    if (old_files == NULL) {
        return 0;
    }

    if (clone_flags & CLONE_FILES) {
        files = old_files;
        goto ok_files;
    }

    int ret = -1;
    if ((files = files_create()) == NULL) {
        ret = -1;
        goto failed_files;
    }

    if ((ret = files_dup(files, old_files)) != 0) {
        goto failed_dup;
    }

ok_files:
    files_ref_inc(files);
    task->files = files;
    return 0;
failed_dup:
    files_destroy(files);
failed_files:
    return ret;
}
static void put_files(Task_t *task) {
    File_t *files = task->files;
    if (files != NULL) {
        if (files_ref_dec(files) <= 0) {
            files_destroy(files);
        }
    }
}

// 设置进程在内核（用户态）正常运行和调度所需的中断帧和执行上下文
// 在进程的内核栈顶设置中断帧
// 设置进程的内核进入点和进程栈
int32_t copy_task(Task_t *task, uint32_t esp, InterruptFrame_t *_if) {
    task->_if = (InterruptFrame_t *)(task->kstack + KSTACKSIZE) - 1;
    *(task->_if) = *_if;
    task->_if->eax = 0; // 子进程的返回值
    task->_if->user_esp = esp;
    SetBit(&(task->_if->eflags), 9); // 打开中断
    task->context.eip = forkret;
    task->context.esp = (uint32_t)task->_if;
}
// 进程终止处理，本函数不能返回
int do_exit(int err_code) {
    if (CurrentTask == idle_task) {
        panic("Trying to exit idle!");
    }
    MemMap_t *mm = CurrentTask->mm;
    if (mm != NULL) {     // 是用户进程
                          //  用户态进程退出
        lcr3(kernel_cr3); // 1 切换到内核页表
        if (mm_ref_dec(mm) ==
            0) { // 2 mm_count-1=0 说明此 mm 没有被其他进程共享,可以被释放
            exit_mmap(mm);
            put_pgdir(mm);
            mm_destroy(mm);
        }
        CurrentTask->mm = NULL; // 表示当前进程的内存已释放完毕
    }
    // put_files(CurrentTask);
    CurrentTask->state = TASK_ZOMBIE;
    CurrentTask->exit_code = err_code;

    bool flag;
    Task_t *ptask;
    intr_save(flag);
    {
        // printk("Task %d:process exit!\n", CurrentTask->tid);
        ptask = CurrentTask->parent;
        if (ptask->wait_state == WT_CHILD) {
            wakeupTask(ptask);
        }

        while (CurrentTask->cptr != NULL) {
            ptask = CurrentTask->cptr;
            CurrentTask->cptr = ptask->optr;

            ptask->yptr = NULL;
            if ((ptask->optr = init_task->cptr) != NULL) {
                init_task->cptr->yptr = ptask;
            }
            ptask->parent = init_task;
            init_task->cptr = ptask;
            if (ptask->state == TASK_ZOMBIE) {
                if (init_task->wait_state == WT_CHILD) {
                    wakeupTask(init_task);
                }
            }
        }
    }
    intr_restore(flag);

    sprintk("进程%d退出!\n", CurrentTask->tid);

    schedule();
    panic("do_exit will not return!");
}

static int load_icode_read(int fd, void *buf, size_t len, int32_t offset) {
    int ret;
    if ((ret = sysfile_seek(fd, offset, LSEEK_SET)) != 0) {
        return ret;
    }
    if ((ret = sysfile_read(fd, buf, len)) != len) {
        return (ret < 0) ? ret : -1;
    }
    return 0;
}
static int load_icode(int fd, int argc, char **kargv) {
    sprintk(" load icode:\n");
    if (CurrentTask->mm != NULL) {
        panic("load_icode: current->mm must be empty.\n");
    }
    int ret = -E_NOMEM;
    MemMap_t *mm;
    if ((mm = mm_create()) == NULL) {
        goto bad_mm;
    }
    if (setup_pgdir(mm) != 0) {
        goto bad_pgdir_cleanup_mm;
    }

    PhyPageBlock_t *page;
    // 从磁盘加载 elf 文件头
    ELF_Header_t __elf, *elf = &__elf;
    if ((ret = load_icode_read(fd, elf, sizeof(ELF_Header_t), 0)) != 0) {
        goto bad_elf_cleanup_pgdir;
    }

    if (elf->e_magic != ELF_MAGIC) {
        ret = -E_INVAL_ELF;
        goto bad_elf_cleanup_pgdir;
    }
    // 从磁盘加载所有 elf program header
    ELF_SectionHeader_t __ph, *ph = &__ph;
    uint32_t vm_flags, perm, phnum;
    for (phnum = 0; phnum < elf->e_phnum; phnum++) {
        sprintk("加载第 %d 个 program, 共 %d 个.\n", phnum + 1, elf->e_phnum);
        int32_t phoff = elf->e_phoff + sizeof(ELF_SectionHeader_t) * phnum;
        if ((ret = load_icode_read(fd, ph, sizeof(ELF_SectionHeader_t),
                                   phoff)) != 0) {
            goto bad_cleanup_mmap;
        }
        sprintk("\t已加载: program header\n");

        /*** elf program header 校验 begin ***/
        if (ph->p_type != ELF_PT_LOAD) {
            continue;
        }
        if (ph->p_filesz > ph->p_memsz) {
            ret = -E_INVAL_ELF;
            goto bad_cleanup_mmap;
        }
        if (ph->p_filesz == 0) {
            continue;
        }
        sprintk("\t已校验: program header\n");
        /*** elf program header 校验 end ***/
        // 根据elf 标志位 确认内存描述符属性
        vm_flags = 0, perm = PTE_PAGE_USER;
        if (ph->p_flags & ELF_PF_X)
            vm_flags |= VMM_EXEC;
        if (ph->p_flags & ELF_PF_W)
            vm_flags |= VMM_WRITE;
        if (ph->p_flags & ELF_PF_R)
            vm_flags |= VMM_READ;
        if (vm_flags & VMM_WRITE)
            perm |= PTE_PAGE_WRITEABLE;
        //
        if ((ret = mm_map(mm, ph->p_va, ph->p_memsz, vm_flags, NULL)) != 0) {
            goto bad_cleanup_mmap;
        }
        sprintk("\t已建立 mm: [0x%08X,0x%08X)\n", ph->p_va,
                ph->p_va + ph->p_memsz);
        int32_t offset = ph->p_offset;
        size_t off, size;
        uint32_t start = ph->p_va, end, la = ROUNDDOWN(start, PGSIZE);

        ret = -E_NOMEM;

        end = ph->p_va + ph->p_filesz;
        while (start < end) {
            if ((page = pgdir_alloc_page(mm->pgdir, la, perm)) == NULL) {
                ret = -E_NOMEM;
                goto bad_cleanup_mmap;
            }
            off = start - la, size = PGSIZE - off, la += PGSIZE;
            if (end < la) {
                size -= la - end;
            }
            if ((ret = load_icode_read(fd, page2kva(page) + off, size,
                                       offset)) != 0) {
                goto bad_cleanup_mmap;
            }
            start += size, offset += size;
        }
        end = ph->p_va + ph->p_memsz;

        if (start < la) {
            /* ph->p_memsz == ph->p_filesz */
            if (start == end) {
                continue;
            }
            off = start + PGSIZE - la, size = PGSIZE - off;
            if (end < la) {
                size -= la - end;
            }
            memset(page2kva(page) + off, 0, size);
            start += size;
            // assert((end < la && start == end) || (end >= la && start == la));
        }
        while (start < end) {
            if ((page = pgdir_alloc_page(mm->pgdir, la, perm)) == NULL) {
                ret = -E_NOMEM;
                goto bad_cleanup_mmap;
            }
            off = start - la, size = PGSIZE - off, la += PGSIZE;
            if (end < la) {
                size -= la - end;
            }
            memset(page2kva(page) + off, 0, size);
            start += size;
        }
        sprintk("\t已建立: 页表\n");
    }
    sysfile_close(fd);

    vm_flags = VMM_READ | VMM_WRITE | VMM_STACK;
    if ((ret = mm_map(mm, USTACK_TOP - USTACK_SIZE, USTACK_SIZE, vm_flags,
                      NULL)) != 0) {
        goto bad_cleanup_mmap;
    }
    assert(pgdir_alloc_page(mm->pgdir, USTACK_TOP - PGSIZE, PTE_USER) != NULL,
           "pgdir_alloc_page err!");
    assert(pgdir_alloc_page(mm->pgdir, USTACK_TOP - 2 * PGSIZE, PTE_USER) !=
               NULL,
           "pgdir_alloc_page err!");
    assert(pgdir_alloc_page(mm->pgdir, USTACK_TOP - 3 * PGSIZE, PTE_USER) !=
               NULL,
           "pgdir_alloc_page err!");
    assert(pgdir_alloc_page(mm->pgdir, USTACK_TOP - 4 * PGSIZE, PTE_USER) !=
               NULL,
           "pgdir_alloc_page err!");

    mm_ref_inc(mm);
    // 安装 mm
    CurrentTask->mm = mm;
    CurrentTask->cr3 = kva2pa(mm->pgdir);
    lcr3(kva2pa(mm->pgdir));

    // 计算 argc, argv
    uint32_t argv_size = 0, i;
    for (i = 0; i < argc; i++) {
        argv_size += strnlen(kargv[i], EXEC_MAX_ARG_LEN + 1) + 1;
    }

    // USTACKTOP 为固定值,每个进程的用户栈范围都相同
    uint32_t stacktop =
        USTACK_TOP - (argv_size / sizeof(long) + 1) * sizeof(long); // 用户栈顶,
    char **uargv = (char **)(stacktop - argc * sizeof(char *));

    argv_size = 0;
    for (i = 0; i < argc; i++) {
        uargv[i] = strcpy((char *)(stacktop + argv_size), kargv[i]);
        argv_size += strnlen(kargv[i], EXEC_MAX_ARG_LEN + 1) + 1;
    }

    stacktop = (uint32_t)uargv - sizeof(int);
    *(int *)stacktop = argc; // 在栈顶处压入参数

    InterruptFrame_t *tf = CurrentTask->_if;
    memset(tf, 0, sizeof(InterruptFrame_t));
    tf->cs = USER_CS;                   // 用户代码段
    tf->ds = tf->es = tf->ss = USER_DS; // 用户数据段
    tf->user_esp = stacktop;            // 用户栈
    sprintk("user stacktop:0x%08X\n", stacktop);
    tf->eip = elf->e_entry; // 此 elf 的入口
    sprintk("user entry:0x%08X\n", elf->e_entry);
    tf->eflags = FL_IF;
    ret = 0;
    sprintk("load_icode ok.\n");

out:
    return ret;
bad_cleanup_mmap:
    exit_mmap(mm);
bad_elf_cleanup_pgdir:
    put_pgdir(mm);
bad_pgdir_cleanup_mm:
    mm_destroy(mm);
bad_mm:
    goto out;
}
// this function isn't very correct
static void put_kargv(int argc, char **kargv) {
    while (argc > 0) {
        // BUG TODO in put_kargv
        kfree(kargv[--argc], EXEC_MAX_ARG_LEN + 1);
    }
}
static int copy_kargv(MemMap_t *mm, int argc, char **kargv, const char **argv) {
    int i, ret = -E_INVAL;
    if (!checkUserMem(mm, (uint32_t)argv, sizeof(const char *) * argc, 0)) {
        return ret;
    }
    for (i = 0; i < argc; i++) {
        char *buffer;
        if ((buffer = kmalloc(EXEC_MAX_ARG_LEN + 1)) == NULL) {
            goto failed_nomem;
        }
        if (!copyLegalStr(mm, buffer, argv[i], EXEC_MAX_ARG_LEN + 1)) {
            kfree(buffer, EXEC_MAX_ARG_LEN + 1);
            goto failed_cleanup;
        }
        kargv[i] = buffer;
    }
    return 0;

failed_nomem:
    ret = -E_NOMEM;
failed_cleanup:
    put_kargv(i, kargv);
    return ret;
}

int do_execve(const char *name, int argc, const char **argv) {
    sprintk("execve start:\n");
    MemMap_t *mm = CurrentTask->mm;
    if (!(argc >= 1 && argc <= EXEC_MAX_ARG_NUM)) {
        return -E_INVAL;
    }
    char local_name[TASK_NAME_LEN + 1];
    memset(local_name, 0, sizeof(local_name));

    char *kargv[EXEC_MAX_ARG_NUM];
    const char *path;

    int ret = -E_INVAL;
    sprintk("e1\n");
    lock_mm(mm);
    if (name == NULL) {
        strcpy(local_name, "null");
    } else {
        if (!copyLegalStr(mm, local_name, name, sizeof(local_name))) {
            unlock_mm(mm);
            return ret;
        }
    }
    sprintk("e2\n");
    if ((ret = copy_kargv(mm, argc, kargv, argv)) != 0) {
        unlock_mm(mm);
        return ret;
    }
    sprintk("e3\n");
    path = argv[0];
    unlock_mm(mm);

    // int fd;
    // if ((ret = fd = sysfile_open(path, OPEN_READ)) < 0) {
    //     sprintk("execve open failed!\n");
    //     goto failed_open;
    // }else{
    //     sprintk("e31\n");
    //     sysfile_close(fd);
    // }

    sprintk("e32\n");
    files_closeall(CurrentTask->files);
    sprintk("e33:closeall\n");

    int fd = -1;
    if ((ret = fd = sysfile_open(path, OPEN_READ)) < 0) {
        sprintk("execve open failed!\n");
        goto execve_do_exit;
    }
    sprintk("e4\n");

    sprintk("e5\n");
    if (mm != NULL) {
        lcr3(kernel_cr3);
        if (mm_ref_dec(mm) == 0) {
            exit_mmap(mm);
            put_pgdir(mm);
            mm_destroy(mm);
        }
        CurrentTask->mm = NULL;
    }
    ret = -E_NOMEM;
    sprintk("e6\n");

    if ((ret = load_icode(fd, argc, kargv)) != 0) {
        goto execve_do_exit;
    }
    sprintk("e7\n");
    put_kargv(argc, kargv);
    setTaskName(CurrentTask, local_name);
    sprintk("e8\n");
    return 0;
failed_open:
    put_kargv(argc, kargv);
    return -E_NOT_EXIST;

execve_do_exit:
    put_kargv(argc, kargv);
    do_exit(ret);
    panic("already exit.");
}
int exec_user(const char *name, const char **argv) {
    int argc = 0;
    int ret;
    while (argv[argc] != NULL) {
        argc++;
    }
    asm volatile("int %1;"
                 : "=a"(ret)
                 : "i"(T_SYSCALL), "0"(SYS_exec), "d"(name), "c"(argc),
                   "b"(argv)
                 : "memory");
    return ret;
}
#define __KERNEL_EXECVE(name, path, ...)                                       \
    ({                                                                         \
        const char *argv[] = {path, ##__VA_ARGS__, NULL};                      \
        exec_user(name, argv);                                                 \
    })

#define KERNEL_EXECVE(x, ...) __KERNEL_EXECVE(x, x, ##__VA_ARGS__)

void test_user() {
    sprintk("\n-----------USER TEST------- -\n");
    KERNEL_EXECVE("/fs0/user/sh");
    // sprintk("\n-----------USER TEST END------- -\n");
}

// 等待一个或多个子进程僵死并清理.
//  一旦子进程变为PROC_ZOMBIE状态,此函数则清理子进程的资源.
// 若 pid 为 0,则用户调用的是 wait()函数
// 若 pid 不为 0 则用户调用的是 waitpid()函数
//
// 考虑因素: 如果子进程还有子进程,则需要睡眠等待.重新调度进程队列.
//
// 清理资源: 1 从内核的进程维护表中移除  2 释放栈空间 释放进程控制块空间
int do_wait(int pid, int *code_store) {
    MemMap_t *mm = CurrentTask->mm;
    if (code_store != NULL) {
        if (!checkUserMem(mm, (uint32_t)code_store, sizeof(int), 1)) {
            return -E_INVAL;
        }
    }

    Task_t *task;
    bool intr_flag, haskid;
repeat:
    haskid = 0;     // 是否有子进程
    if (pid != 0) { // 若调用的是 waitpid()
        task = find_task(pid);
        if (task != NULL &&
            task->parent == CurrentTask) { // 若找到的进程是当前进程的子进程,
            haskid = 1;
            if (task->state == TASK_ZOMBIE) { // 必须是僵死状态才可回收
                goto found;
            }
        }
    } else { // 调用的是 wait(),则找到任意一个处于退出状态的子进程
        task = CurrentTask->cptr;
        for (; task != NULL; task = task->optr) {
            haskid = 1;
            if (task->state == TASK_ZOMBIE) {
                goto found;
            }
        }
    }
    if (haskid) {
        CurrentTask->state = TASK_SLEEPING;
        CurrentTask->wait_state = WT_CHILD;
        schedule();
        if (CurrentTask->flags & PF_EXITING) {
            do_exit(-E_KILLED);
        }
        goto repeat;
    }
    return -E_BAD_PROC;

found:
    if (task == idle_task || task == init_task) {
        panic("Can not wait idleproc or initproc!\n");
    }
    if (code_store != NULL) {
        *code_store = task->exit_code;
    }
    intr_save(intr_flag);
    { del_task(task); }
    intr_restore(intr_flag);
    freeKstack(task);
    kfree(task, sizeof(Task_t));
    return 0;
}