#ifndef TIMER_H
#define TIMER_H
#include "types.h"

uint32_t Timer_SysTick; //系统定时器Tick数
uint32_t Timer_SysSecond;

void initTimer();

#endif