#include "stdarg.h"
#include "console.h"
#include "debug.h"
#include "elf.h"
#include "multiboot.h"
#include "pmm.h"
#include "vmm.h"

extern int vsprintf(char* buf, const char* fmt, va_list args); //lib/vsprintf.c
static ELF_t kernel_elf;//内核ELF结构，跟踪栈帧用

void initDebug() {
    //consoleWriteColor("Init Debug Unit...", TC_black, TC_light_blue);
    kernel_elf = ELF_FromMultiBoot(GlbMbootPtr);
    //consoleWriteColor("OK\n", TC_black, TC_yellow);
}
void printSegStatus() {
    static int round = 0;
    uint16_t reg0, reg1, reg2, reg3;
    uint32_t reg4;

    asm volatile (
        "mov %%cs, %0;"
        "mov %%ds, %1;"
        "mov %%es, %2;"
        "mov %%ss, %3;"
        "mov %%cr0, %%eax;"
        "movl %%eax, %4"
        : "=m"(reg0), "=m"(reg1), "=m"(reg2), "=m"(reg3), "=m"(reg4)
        :
        : "%eax");

    printk("Register Data and CPU Rings:\n");
    printk("%d:  cs = 0x%x\n", round, reg0);
    printk("%d:  ds = 0x%x\n", round, reg1);
    printk("%d:  es = 0x%x\n", round, reg2);
    printk("%d:  ss = 0x%x\n", round, reg3);
    printk("%d: cr0 = 0x%x\n", round, reg4);
    printk("%d: @ring %d\n\n", round, reg0 & 0x3);
    ++round;
}
void printk(const char* format, ...) {
    static char buf[512];
    va_list args;
    int i;

    va_start(args, format);
    i = vsprintf(buf, format, args);
    va_end(args);

    buf[i] = '\0';

    consoleWrite(buf);
    //printkColor(buf,TC_black,TC_white);
}
void printkColor(const char* format, TEXT_color_t back, TEXT_color_t fore, ...) {
    static char buf[512];
    va_list args;
    int i;

    va_start(args, fore);
    i = vsprintf(buf, format, args);
    va_end(args);

    buf[i] = '\0';

    consoleWriteColor(buf, back, fore);
}
inline void panic(const char* msg) {
    printk("\n*** System Panic: %s ***\n", msg);
    //printStackTrace();
    printk("***\n");

    // 致命错误发生后循环
    while (1) {
        asm("hlt");
    }
}
void printStackTrace() {
    uint32_t* ebp, * eip;

    asm volatile ("mov %%ebp, %0" : "=r" (ebp));
    while (ebp) {
        eip = ebp + 1;
        printk("   [0x%x] %s\n", *eip, ELF_SymbolLookup(*eip, &kernel_elf));
        ebp = (uint32_t*) *ebp;
    }
}
void printKernelMemStauts() {
    uint32_t mmap_addr = GlbMbootPtr->mmap_addr + KERNEL_OFFSET;//分页后需要加上偏移
    uint32_t mmap_length = GlbMbootPtr->mmap_length;

    printk("Kernel Memory Stauts:\n");
    printk("Kernel in pmemory start: 0x%08X\n", kern_start - KERNEL_OFFSET);
    printk("Kernel in pmemory end:   0x%08X\n", kern_end - KERNEL_OFFSET);
    printk("Kernel in memory used:  %dB, %dKB\n\n", kern_end - kern_start + 1023, (kern_end - kern_start + 1023) / 1024);

    printk("Kernel Memory Map:\n");
    mmapEntry_t* mmap = (mmapEntry_t*) mmap_addr;
    for (mmap = (mmapEntry_t*) mmap_addr;(uint32_t) mmap < mmap_addr + mmap_length;mmap++) {
        printk("base_addr = 0x%X%08X, length = 0x%X%08X, type = 0x%X\n",
            (uint32_t) mmap->base_addr_high, (uint32_t) mmap->base_addr_low,
            (uint32_t) mmap->length_high, (uint32_t) mmap->length_low,
            (uint32_t) mmap->type);
    }
}
