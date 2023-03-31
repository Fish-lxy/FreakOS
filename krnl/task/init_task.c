#include "debug.h"
#include "gdt.h"
#include "ide.h"
#include "idt.h"
#include "kbd.h"
#include "list.h"
#include "pmm.h"
#include "string.h"
#include "task.h"
#include "types.h"
#include "vmm.h"

#include "fat_fs.h"
#include "partiton.h"

void testA() {
    int i = 0;
    while (1) {
        if (i % 1000000 == 0) {
            printk("A");
        }
        i++;
    }
}
void testB() {
    int i = 0;
    while (1) {
        if (i % 1000000 == 0) {
            printk("B");
        }
        i++;
    }
}

bool flag = TRUE;
extern BlockDev_t MainBlockdev;
// 系统的第二个进程 init
int init_task_func(void *arg) {
    // init进程将会接手 main 函数的工作，继续内核的初始化
    printk("Start init process:\n");
    initBlockdev();
    initPartitionTable(); // 从MBR中读取并初始化磁盘分区表信息

    testFatFs();
    initKBD();

    createKernelThread(testA, NULL, 0);
    createKernelThread(testB, NULL, 0);

    testRunQueue();

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
// 测试进程
int test_task(void *arg) {
    // printk("test process:\n");
    // printk("B\n");
    // flag = TRUE;

    // printk("To U: %s\n", (const char*) arg);

    // int i = 0;
    // while (1) {

    //     if (i % 1000000 == 0) {
    //         printk("B");
    //         i = 0;
    //     }
    //     i++;
    // }
    while (1)
        ;
    return 0;
}
