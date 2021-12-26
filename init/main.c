#include "types.h"
#include "console.h"
#include "debug.h"
#include "gdt.h"
#include "idt.h"
#include "timer.h"
#include "multiboot.h"
#include "cpu.h"
#include "pmm.h"
#include "vmm.h"

void int3();
void cpu_idle();
void main();

void main() {
	consoleClear();
	//printk(" Paging Enabled.\n");

	initDebug();
	initGDT();
	initIDT();

	initPMM();
	initVMM();

	initTask();
	initTimer();

	printk("\nLoading LiteOS Kernel...\n");
	//printSegStatus();

	//printKernelMemStauts();

	// asm volatile ("int $0x03");
	// interruptHandlerRegister(3,int3);
	// asm volatile ("int $0x03");

	//printk("%d\n",read_eflags());

	
	sti();//允许中断
	
	//此条 main 执行流将会蜕变为第 0号进程 idle
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
		asm volatile("hlt");
	}
}
void int3() {
	consoleWriteColor("Interrupt Called! intcode : 3\n", TC_black, TC_green);
}
