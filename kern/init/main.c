#include "console.h"
#include "cpu.h"
#include "debug.h"
#include "gdt.h"
#include "idt.h"
#include "multiboot.h"
#include "pmm.h"
#include "sched.h"
#include "serial.h"
#include "syscall.h"
#include "task.h"
#include "timer.h"
#include "types.h"
#include "vmm.h"

#include "kmalloc.h"

void int3();
void cpu_idle();
void main();

void main() {
    consoleClear();
    // printk(" Paging Enabled.\n");

    initKernelELF_Info();

    initGDT();
    initIDT();
    initSysCall();

    initDebugSerial();

    initPMM();
    initKernelPageTable();

    //test_pgdir();
    //while(1){}
    initTask();
    initTimer();

    
    
    sprintk("Loading FreakOS Kernel...\n");

    printk("\nLoading FreakOS Kernel...\n");

    // testKmalloc();
    // printSegStatus();

    // printKernelMemStauts();

    // asm volatile ("int $0x03");
    // intrHandlerRegister(3,int3);
    // asm volatile ("int $0x03");

    // printk("%d\n",read_eflags());

    sti(); // 允许中断

    // 此条 main 执行流将会蜕变为第 0 号进程 idle
    cpu_idle();
    // int i = 0;
    // while (1) {
    //     if (i % 2000000 == 0) {
    //         printk("C");
    // 		i=0;
    //     }
    //     i++;
    // }
}
void cpu_idle() {
    while (1) {
        //asm volatile("hlt");
        if (CurrentTask->need_resched != 0) {
            schedule();
        }
    }
}
void int3() {
    consoleWriteStrColor("Interrupt Called! intcode : 3\n", TC_black, TC_green);
}
