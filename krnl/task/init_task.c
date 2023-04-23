#include "bcache.h"
#include "debug.h"
#include "gdt.h"
#include "ide.h"
#include "idt.h"
#include "kbd.h"
#include "list.h"
#include "pmm.h"
#include "sem.h"
#include "string.h"
#include "task.h"
#include "types.h"
#include "vmm.h"
#include "vfs.h"

#include "fat.h"
#include "partiton.h"

volatile uint32_t sum = 0;
Semaphore_t sem;
void testA() {
    int i = 0;
    // while (1) {
    //     if (i % 25000000 == 0) {
    //         printk("A");
    //     }
    //     i++;
    // }

    // for(int i=0;i<200;i++){
    //     sum++;
    //     printk(" A:%d\n",sum);

    // }
    // printk("A:%d\n",sum);

    // initSem(&sem, 0);

    // accquireSem(&sem);
    // printk("second\n");
}
void testB() {
    int i = 0;
    // while (1) {
    //     if (i % 25000000 == 0) {
    //         printk("B");
    //     }
    //     i++;
    // }

    // for(int i=0;i<200;i++){
    //     sum++;
    //     printk(" B:%d\n",sum);

    // }
    // printk("B:%d\n",sum);

    // printk("first\n");
    // releaseSem(&sem);
}

// 系统的第二个进程 init
int init_task_func(void *arg) {
    // init内核进程将会接手 main 函数的工作，继续内核的初始化
    sprintk("init进程开始：\n");
    printk("Start init process:\n");

    initKBD();
    initDevice();
    initPartitionTable(); // 从MBR中读取并初始化磁盘分区表信息
    initBlockCache();




    
    initFS();
    testVFS();
    testFatInode();
    

    //createKernelThread(testA, NULL, 0);
    //createKernelThread(testB, NULL, 0);

    //testBCache();
    
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
    printk("OK.\n");

    while (1) {
        asm("hlt");
    }
}
