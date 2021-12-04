#include "timer.h"
#include "types.h"
#include "idt.h"
#include "io.h"
#include "debug.h"

#define FREQUENCY 100

void timerCallBack() {
    Timer_SysTick++;
    if(Timer_SysTick % 100 == 0){
        Timer_SysSecond++;
        //printk("Time: %d\r",Timer_SysSecond);
    }
}
void initTimer() {
    //consoleWriteColor("Init Timer...", TC_black, TC_light_blue);
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
    uint8_t low = (uint8_t) (divisor & 0xFF);
    uint8_t hign = (uint8_t) ((divisor >> 8) & 0xFF);

    // 分别写入低字节和高字节
    outb(0x40, low);
    outb(0x40, hign);
    //consoleWriteColor("OK\n", TC_black, TC_yellow);
}