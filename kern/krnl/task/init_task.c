#include "bcache.h"
#include "debug.h"
#include "fat.h"
#include "file.h"
#include "gdt.h"
#include "ide.h"
#include "idt.h"
#include "kbd.h"
#include "kmalloc.h"
#include "list.h"
#include "mm.h"
#include "partiton.h"
#include "pmm.h"
#include "sem.h"
#include "string.h"
#include "syscall.h"
#include "task.h"
#include "types.h"
#include "vdev.h"
#include "vfs.h"
#include "vmm.h"

volatile uint32_t sum = 0;
Semaphore_t sem;
extern uint8_t idtflag(uint8_t istrap, uint8_t dpl);

int i;
void user_task() {
    // printk("idtflag:%X\n",idtflag(FALSE, DPL_KERNEL));

    //  while (1) {
    //      if (i % 1000000 == 0) {
    //          sprintk("B");
    //      }
    //      i++;
    //  }

    // for(int i=0;i<200;i++){
    //     sum++;
    //     printk(" A:%d\n",sum);

    // }
    // printk("A:%d\n",sum);

    // initSem(&sem, 0);

    // acquireSem(&sem);
    // printk("second\n");

    //test_putc_syscall('A');
    test_user();

    // printk("Physical Mem Test:\n");
    // char* i = kmalloc(8);
    // printk("alloc mem: %dB in 0x%08X\n",8,i);
    // char* ii = kmalloc(16);
    // printk("alloc mem: %dB in 0x%08X\n",16,ii);
    // char* iii = kmalloc(32);
    // printk("alloc mem: %dB in 0x%08X\n",32,iii);

    // printk("free mem: %dB in 0x%08X\n",8,i);
    // kfree(i,8);
    // printk("free mem: %dB in 0x%08X\n",16,ii);
    // kfree(ii,16);
    // printk("free mem: %dB in 0x%08X\n",32,iii);
    // kfree(iii,32);
}
void testB() {
    // while (1) {
    //      if (i % 1000000 == 0) {
    //          sprintk("A");
    //      }
    //      i++;
    //  }
    //test_file();

    test_putc_syscall('B');

    // char *str = kmalloc(sizeof(char) * 2048);
    // memset(str, 0, 2048);
    // while (1) {
    //     printk("$ ");
    //     test_vdev_readline(str);
    //     printk("stdin:%s\n", str);
    // }

    // for(int i=0;i<200;i++){
    //     sum++;
    //     printk(" B:%d\n",sum);

    // }
    // printk("B:%d\n",sum);

    // printk("first\n");
    // releaseSem(&sem);
}
void sync_bcache_task();

bool KernelReady = FALSE;
// 系统的第二个进程 init
int init_task_func(void *arg) {
    // init内核进程将会接手 main 函数的工作，继续内核的初始化
    sprintk("init进程开始：\n");
    printk("Start init process:\n");

    initKBD();
    initBlockDevice();
    initPartitionTable(); // 从MBR中读取并初始化磁盘分区表信息
    initBlockCache();

    initFS();

    init_vDev();

    // createKernelThread(sync_bcache_task, NULL, 0);

    // testBCache();

    // testRunQueue();

    // printk("Msg: %s\n", (const char*) arg);

    // int i = 0;
    // while (1) {

    //     if (i % 1000000 == 0) {
    //         printk("A");
    //         i = 0;
    //     }
    //     i++;
    // }
    printFreeMem();

    KernelReady = TRUE;
    printk("OK.\n");

    //-----TEST-------------------------------------------------------------
    //sprintk("\n\nALL TEST:\n\n");
    //printk("ALL TEST:\n");

    //test_vfs();
    //testFatInode();

    //test_vDev();

    //test_vfslookup();
    //test_vfs_file();

    //test_vma();
    //test_pgfault();
     //test_file();

    //createKernelThread(testB, NULL, 0);
    //-----------------------------------------------------------------------

    printk("\nStart user task...\n");
    createKernelThread(user_task, NULL, 0);
    // 作为所有子进程的父进程,清理所有子进程资源
    while (do_wait(0, NULL) == 0) { 
        schedule();
    }
}
