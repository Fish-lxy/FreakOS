#include "timer.h"
#include "cpu.h"
#include "debug.h"
#include "idt.h"
#include "sched.h"
#include "task.h"
#include "types.h"

#define FREQUENCY 100

uint32_t volatile Timer_SysTick; //系统定时器Tick数
uint32_t Timer_SysSecond;

void timerCallBack() {
    Timer_SysTick++;
    if (Timer_SysTick % FREQUENCY == 0) {
        Timer_SysSecond++;
        // printk("Time: %d\n",Timer_SysSecond);
    }
    tickCurrentTask();
    if (CurrentTask->need_resched == 1) {
        schedule();
    }
}
void initTimer() {
    interruptHandlerRegister(IRQ0, timerCallBack);

    // Intel 8253/8254 PIT芯片 I/O端口地址范围是40h~43h
    // 输入频率为 1193180，FREQUENCY 即每秒中断次数
    uint32_t divisor = 1193180 / FREQUENCY;

    // D7 D6 D5 D4 D3 D2 D1 D0
    // 0  0  1  1  0  1  1  0
    // 36 H
    // 设置 8253/8254 芯片工作在模式 3 下
    outb(0x43, 0x36);

    // 拆分低字节和高字节
    uint8_t low = (uint8_t)(divisor & 0xFF);
    uint8_t high = (uint8_t)((divisor >> 8) & 0xFF);

    // 分别写入低字节和高字节
    outb(0x40, low);
    outb(0x40, high);
}