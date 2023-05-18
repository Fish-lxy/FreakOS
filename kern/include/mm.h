#ifndef __MM_H
#define __MM_H

extern uint8_t
    kern_start[]; // 由链接器提供内核的起始虚拟地址(/script/kernel.ld)
extern uint8_t kern_end[];

// global segment number
#define SEG_KTEXT 1
#define SEG_KDATA 2
#define SEG_UTEXT 3
#define SEG_UDATA 4
#define SEG_TSS 5

// global descrptor numbers
#define GD_KTEXT ((SEG_KTEXT) << 3) // kernel text 0x8
#define GD_KDATA ((SEG_KDATA) << 3) // kernel data 0x10 16
#define GD_UTEXT ((SEG_UTEXT) << 3) // user text 0x18 24
#define GD_UDATA ((SEG_UDATA) << 3) // user data 0x20  32
#define GD_TSS ((SEG_TSS) << 3)     // task segment selector 0x28 40

#define DPL_KERNEL (0)
#define DPL_USER (3)

#define KERNEL_CS ((GD_KTEXT) | DPL_KERNEL) // 0x8
#define KERNEL_DS ((GD_KDATA) | DPL_KERNEL) // 0x10 16
#define USER_CS ((GD_UTEXT) | DPL_USER)     // 0x1B 27
#define USER_DS ((GD_UDATA) | DPL_USER)     // 0x23 35

// Eflags register
#define FL_CF 0x00000001        // Carry Flag
#define FL_PF 0x00000004        // Parity Flag
#define FL_AF 0x00000010        // Auxiliary carry Flag
#define FL_ZF 0x00000040        // Zero Flag
#define FL_SF 0x00000080        // Sign Flag
#define FL_TF 0x00000100        // Trap Flag
#define FL_IF 0x00000200        // Interrupt Flag
#define FL_DF 0x00000400        // Direction Flag
#define FL_OF 0x00000800        // Overflow Flag
#define FL_IOPL_MASK 0x00003000 // I/O Privilege Level bitmask
#define FL_IOPL_0 0x00000000    //   IOPL == 0
#define FL_IOPL_1 0x00001000    //   IOPL == 1
#define FL_IOPL_2 0x00002000    //   IOPL == 2
#define FL_IOPL_3 0x00003000    //   IOPL == 3
#define FL_NT 0x00004000        // Nested Task
#define FL_RF 0x00010000        // Resume Flag
#define FL_VM 0x00020000        // Virtual 8086 mode
#define FL_AC 0x00040000        // Alignment Check
#define FL_VIF 0x00080000       // Virtual Interrupt Flag
#define FL_VIP 0x00100000       // Virtual Interrupt Pending
#define FL_ID 0x00200000        // ID flag

// Application segment type bits
#define STA_X 0x8 // Executable segment
#define STA_E 0x4 // Expand down (non-executable segments)
#define STA_C 0x4 // Conforming code segment (executable only)
#define STA_W 0x2 // Writeable (non-executable segments)
#define STA_R 0x2 // Readable (executable segments)
#define STA_A 0x1 // Accessed

// System segment type bits
#define STS_T16A 0x1 // Available 16-bit TSS
#define STS_LDT 0x2  // Local Descriptor Table
#define STS_T16B 0x3 // Busy 16-bit TSS
#define STS_CG16 0x4 // 16-bit Call Gate
#define STS_TG 0x5   // Task Gate / Coum Transmitions
#define STS_IG16 0x6 // 16-bit Interrupt Gate
#define STS_TG16 0x7 // 16-bit Trap Gate
#define STS_T32A 0x9 // Available 32-bit TSS
#define STS_T32B 0xB // Busy 32-bit TSS
#define STS_CG32 0xC // 32-bit Call Gate
#define STS_IG32 0xE // 32-bit Interrupt Gate
#define STS_TG32 0xF // 32-bit Trap Gate
//-----------------------------------------------

// 支持的最大物理内存(1GB)
#define MAX_PHY_MEM_SIZE 0x38000000 // 896MB
#define KSTACKSIZE 8192

//-----------------------------------------------
typedef uint32_t PGD_t; // 页目录类型
typedef uint32_t PTE_t; // 页表类型

// 页大小(4KB)
#define PGSIZE 0x1000
#define PAGE_MASK 0xFFFFF000

#define PTE_PAGE_PERSENT 0x1
#define PTE_PAGE_WRITEABLE 0x2
#define PTE_PAGE_USER 0x4

#define PTE_USER        (PTE_PAGE_USER | PTE_PAGE_WRITEABLE | PTE_PAGE_PERSENT)

// 每个一级页目录包含的项数,即一个一级页表维护 1024 * 4M = 4G
#define PGD_ECOUNT (PGSIZE / sizeof(PTE_t)) // 1024
// 每个二级页表包含的项数,即每个一级页表项维护的二级页表维护 1024*4K=4M
#define PTE_ECOUNT (PGSIZE / sizeof(uint32_t)) // 1024

#define PGD_PHY_ECOUNT (MAX_PHY_MEM_SIZE / 1024 / 4096) // 256
// 每个一级页表映射的字节数 4096 * 1024  = 4M
#define PGDE_MAP_NBYTE (PTE_ECOUNT * PGSIZE)

//----------------------------------------------------
// 1PGD包含1024个PTE,映射4MB内存地址
// 1PTE映射12位/4KB内存地址

// 获得一个虚拟地址的页目录项索引
#define PGD_INDEX(x) ((((uint32_t)(x)) >> 22) & 0x3FF)
// 获得一个虚拟地址的页表项索引
#define PTE_INDEX(x) ((((uint32_t)(x)) >> 12) & 0x3FF)
// 获得一个虚拟地址的页内偏移
#define PG_OFFSET(x) ((uint32_t)(x)&0xFFF)

// 用高 10 位,中 10 位,低 12 位三部分构建出线性地址
#define PGADDR(d, t, o) ((uintptr_t)((d) << 22 | (t) << 12 | (o)))

// 取出页表项中的包含的地址部分.即取低高 20 位
#define PTE_ADDR(pte) ((uint32_t)(pte) & ~0xFFF)
#define PGD_ADDR(pde) PTE_ADDR(pde)

//---------------------------------------------------
// 内核起始终止虚拟地址偏移
#define KERNEL_BASE 0xC0000000
#define KERNEL_TOP (KERNEL_BASE + MAX_PHY_MEM_SIZE)

#define USER_TOP 0xB0000000
#define USTACK_TOP USER_TOP
#define USTACK_PGS 256
#define USTACK_SIZE (256 * PGSIZE)

#define USER_BASE 0x00200000
#define UTEXT 0x00800000
#define USTAB USER_BASE

#endif