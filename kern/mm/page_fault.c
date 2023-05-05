#include "types.h"
#include "pmm.h"
#include "idt.h"
#include "multiboot.h"
#include "debug.h"

void pageFault(InterruptFrame_t* regs) {
    uint32_t cr2;
    asm volatile ("mov %%cr2, %0" : "=r" (cr2));

    printk("Page fault!\n at 0x%X, virtual address 0x%X\n", regs->eip, cr2);
    printk(" Error code: %x\n", regs->err_code);

    // bit 0 为 0 指页面不存在内存里
    if (!(regs->err_code & 0x1)) {
        printk(" the page wasn't present.\n");
    }
    // bit 1 为 0 表示读错误，为 1 为写错误
    if (regs->err_code & 0x2) {
        printk(" Write error.\n");
    }
    else {
        printk(" Read error.\n");
    }
    // bit 2 为 1 表示在用户模式打断的，为 0 是在内核模式打断的
    if (regs->err_code & 0x4) {
        printk(" In user mode.\n");
    }
    else {
        printk(" In kernel mode.\n");
    }
    // bit 3 为 1 表示错误是由保留位覆盖造成的
    if (regs->err_code & 0x8) {
        printk(" Reserved bits being overwritten.\n");
    }
    // bit 4 为 1 表示错误发生在取指令的时候
    if (regs->err_code & 0x10) {
        printk(" The fault occurred during an instruction fetch.\n");
    }

    //todo
    //panic("fatal memory error!");
    while(1){}
    
}