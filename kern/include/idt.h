#ifndef __IDT_H
#define __IDT_H
#include "types.h"

#define IRQ0 32
#define IRQ_SLAVE 2

#define PIC1 0x20
#define PIC2 0xA0

#define PIC1_CMD PIC1
#define PIC2_CMD PIC2
#define PIC1_DATA (PIC1 + 1)
#define PIC2_DATA (PIC2 + 1)

#define PIC_EOI 0x20

typedef struct IDT_Flag_t{
    unsigned gd_type : 4;           // type(STS_{TG,IG32,TG32})
    unsigned gd_s : 1;              // must be 0 (system)
    unsigned gd_dpl : 2;            // descriptor(meaning new) privilege level
    unsigned gd_p : 1;  
} __attribute__((packed)) IDT_Flag_t;

// 中断描述符
typedef struct IDT_Entry_t {
    uint16_t base_low;     // 中断处理函数地址低位
    uint16_t seg_selector; // 目标代码段描述符选择子
    uint8_t always0;       // 0 段
    uint8_t flags;         // 标志段
    
    uint16_t base_high;    // 中断处理函数地址高位
} __attribute__((packed)) IDT_Entry_t;

// IDTR
typedef struct IDT_Ptr_t {
    uint16_t limit; // 长度
    uint32_t base;  // 基址
} __attribute__((packed)) IDT_Ptr_t;

// 中断帧
// 中断时需要保护的寄存器集合
typedef struct InterruptFrame_t {
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds; // 用于保存用户的数据段描述符

    // 以下寄存器值由 pusha 指令压入栈，从 edi 到 eax
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    uint32_t int_no; // 中断号

    // 以下寄存器值由处理器自动压入
    uint32_t err_code; // 错误代码(有中断错误代码的中断会由CPU压入)
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    // 当发生了特权级的转换，以下寄存器值将被压入
    uint32_t user_esp;
    uint32_t ss;
    // 从这里开始 push
} InterruptFrame_t;

// 定义中断处理函数指针
typedef void (*InterruptHandler_t)(InterruptFrame_t *);

// 注册中断处理函数
void intrHandlerRegister(uint8_t n, InterruptHandler_t h);

// 设置中断描述符
void IDT_setGate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);

// 初始化中断描述符表
void initIDT();

// 0-19 属于 CPU 异常中断，由 ISR 处理
// ISR:中断服务程序(Interrupt Service Routine)

// 调用ISR中断服务程序，汇编中调用
void ISR_handlerCall(InterruptFrame_t *pr);

// 声明中断处理函数
void isr0();  // 0 #DE 除 0 异常
void isr1();  // 1 #DB 调试异常
void isr2();  // 2 NMI
void isr3();  // 3 BP 断点异常
void isr4();  // 4 #OF 溢出
void isr5();  // 5 #BR 对数组的引用超出边界
void isr6();  // 6 #UD 无效或未定义的操作码
void isr7();  // 7 #NM 设备不可用(无数学协处理器)
void isr8();  // 8 #DF 双重故障(有错误代码)
void isr9();  // 9 协处理器跨段操作
void isr10(); // 10 #TS 无效TSS(有错误代码)
void isr11(); // 11 #NP 段不存在(有错误代码)
void isr12(); // 12 #SS 栈错误(有错误代码)
void isr13(); // 13 #GP 常规保护(有错误代码)
void isr14(); // 14 #PF 页故障(有错误代码)
void isr15(); // 15 CPU 保留
void isr16(); // 16 #MF 浮点处理单元错误
void isr17(); // 17 #AC 对齐检查
void isr18(); // 18 #MC 机器检查
void isr19(); // 19 #XM SIMD(单指令多数据)浮点异常

// 20-31 Intel 保留中断
void isr20();
void isr21();
void isr22();
void isr23();
void isr24();
void isr25();
void isr26();
void isr27();
void isr28();
void isr29();
void isr30();
void isr31();

// 32～255 用户自定义中断
void isr170(); // 0xAA

// 32-47 属于外设中断请求，由 IRQ 处理
// 外设的所有中断由中断控制芯片8259A统一汇集之后连接到CPU的INTR引脚
// IRQ:中断请求(Interrupt Request)

// 调用IRQ中断服务程序，汇编中调用
void IRQ_handlerCall(InterruptFrame_t *pr);

void enableIRQ(uint8_t irq);

uint8_t idtflag(uint8_t istrap, uint8_t dpl);

// 定义IRQ
#define IRQ_OFFSET 32

#define IRQ0 32  // 系统计时器
#define IRQ1 33  // 键盘
#define IRQ2 34  // 与 IRQ9 相接，MPU-401 MD 使用
#define IRQ3 35  // 串口设备
#define IRQ4 36  // 串口设备
#define IRQ5 37  // 建议声卡使用
#define IRQ6 38  // 软驱传输控制使用
#define IRQ7 39  // 打印机传输控制使用
#define IRQ8 40  // 即时时钟
#define IRQ9 41  // 与 IRQ2 相接，可设定给其他硬件
#define IRQ10 42 // 建议网卡使用
#define IRQ11 43 // 建议 AGP 显卡使用
#define IRQ12 44 // 接 PS/2 鼠标，也可设定给其他硬件
#define IRQ13 45 // 协处理器使用
#define IRQ14 46 // IDE0 传输控制使用
#define IRQ15 47 // IDE1 传输控制使用

// 声明 IRQ 函数
void irq0();  // 电脑系统计时器
void irq1();  // 键盘
void irq2();  // 与 IRQ9 相接，MPU-401 MD 使用
void irq3();  // 串口设备
void irq4();  // 串口设备
void irq5();  // 建议声卡使用
void irq6();  // 软驱传输控制使用
void irq7();  // 打印机传输控制使用
void irq8();  // 即时时钟
void irq9();  // 与 IRQ2 相接，可设定给其他硬件
void irq10(); // 建议网卡使用
void irq11(); // 建议 AGP 显卡使用
void irq12(); // 接 PS/2 鼠标，也可设定给其他硬件
void irq13(); // 协处理器使用
void irq14(); // IDE0 传输控制使用
void irq15(); // IDE1 传输控制使用

#endif