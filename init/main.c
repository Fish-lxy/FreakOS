#include "types.h"
#include "console.h"
#include "debug.h"
#include "gdt.h"
#include "idt.h"
#include "timer.h"
#include "multiboot.h"
#include "pmm.h"
#include "vmm.h"

void int3();
void main();

void main() {
	consoleClear();
	consoleWriteColor(" Paging Enabled.\n", TC_black, TC_light_blue);

	initDebug();
	initGDT();
	initIDT();
	initVMM();
	initPMM();
	initTimer();

	consoleWriteColor("Loading LiteOS Kernel...\n", TC_black, TC_light_blue);

	//printSegStatus();

	//printKernelMemStauts();

	// asm volatile ("int $0x03");
	// interruptHandlerRegister(3,int3);
	// asm volatile ("int $0x03");
	asm volatile ("sti"); //允许中断

	printk("ok");
	//panic("114514");
	while (1) {
		asm volatile("hlt");
	}
	return 0;
}
void int3() {
	consoleWriteColor("Interrupt Called! intcode : 3\n", TC_black, TC_green);
}
