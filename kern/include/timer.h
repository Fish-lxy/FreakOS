#ifndef __TIMER_H
#define __TIMER_H
#include "types.h"

extern uint32_t volatile Timer_SysTick; //系统定时器Tick数
extern uint32_t Timer_SysSecond;

void initTimer();

#endif