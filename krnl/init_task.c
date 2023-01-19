#include "types.h"
#include "task.h"
#include "pmm.h"
#include "vmm.h"
#include "idt.h"
#include "gdt.h"
#include "list.h"
#include "string.h"
#include "debug.h"

#include "ide.h"
#include "mbr.h"
#include "fat.h"

bool flag = TRUE;
extern BlockDev_t main_blockdev;
// 系统的第二个进程 init
int init_task_func(void* arg) {
    //init进程将会接手 main 函数的工作，继续内核的初始化
    printk("Start init process:\n");
    initBlockdev();
    initMBR();
    initFAT();

    //printk("Msg: %s\n", (const char*) arg);

    // int i = 0;
    // while (1) {

    //     if (i % 1000000 == 0) {
    //         printk("A");
    //         i = 0;
    //     }
    //     i++;
    // }
    printk("OK.\n");


    
    //ide_read_secs(0, 0, data, 1);
    for (int i = 0;i < 512;i++) {
        // if (i % 16 == 0) {
        //     printk("\n");
        // }
        // printk("%02X ", data[i]);
    }

    while (1);
    return 0;
}
// 测试进程
int test_task(void* arg) {
    // printk("test process:\n");
    // printk("B\n");
    // flag = TRUE;


    //printk("To U: %s\n", (const char*) arg);

    // int i = 0;
    // while (1) {

    //     if (i % 1000000 == 0) {
    //         printk("B");
    //         i = 0;
    //     }
    //     i++;
    // }
    while (1);
    return 0;
}