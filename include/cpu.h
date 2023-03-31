#ifndef __CPU_H
#define __CPU_H
#include "types.h"

// 向端口写一个字节
static inline void outb(uint16_t port, uint8_t value) {
    asm volatile("outb %1, %0" : : "dN"(port), "a"(value));
}

// 向端口写一个字
static inline void outw(uint16_t port, uint16_t data) {
    asm volatile("outw %0, %1" : : "a"(data), "d"(port) : "memory");
}

static inline void outsl(uint32_t port, const void *addr, int cnt) {
    asm volatile("cld;"
                 "repne; outsl;"
                 : "=S"(addr), "=c"(cnt)
                 : "d"(port), "0"(addr), "1"(cnt)
                 : "memory", "cc");
}

// 从端口读一个字节
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "dN"(port));
    return ret;
}

// 从端口读一个字
static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile("inw %1, %0" : "=a"(ret) : "dN"(port));
    return ret;
}

static inline void insl(uint32_t port, void *addr, int cnt) {
    asm volatile("cld;"
                 "repne; insl;"
                 : "=D"(addr), "=c"(cnt)
                 : "d"(port), "0"(addr), "1"(cnt)
                 : "memory", "cc");
}

// 允许中断
static inline void sti(void) { asm volatile("sti"); }

// 屏蔽中断
static inline void cli(void) { asm volatile("cli" ::: "memory"); }

// 装载任务状态段寄存器TR
static inline void ltr(uint16_t sel) {
    asm volatile("ltr %0" ::"r"(sel) : "memory");
}

static inline void lcr0(uint32_t cr0) {
    asm volatile("mov %0, %%cr0" ::"r"(cr0) : "memory");
}
static inline void lcr3(uint32_t cr3) {
    asm volatile("mov %0, %%cr3" ::"r"(cr3) : "memory");
}
static inline uint32_t rcr0(void) {
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0)::"memory");
    return cr0;
}

static inline uint32_t rcr1(void) {
    uint32_t cr1;
    asm volatile("mov %%cr1, %0" : "=r"(cr1)::"memory");
    return cr1;
}

static inline uint32_t rcr2(void) {
    uint32_t cr2;
    asm volatile("mov %%cr2, %0" : "=r"(cr2)::"memory");
    return cr2;
}

static inline uint32_t rcr3(void) {
    uint32_t cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3)::"memory");
    return cr3;
}

static inline uint32_t read_eflags(void) {
    uint32_t eflags;
    asm volatile("pushfl; popl %0" : "=r"(eflags));
    return eflags;
}

static inline void write_eflags(uint32_t eflags) {
    asm volatile("pushl %0; popfl" ::"r"(eflags));
}

#endif