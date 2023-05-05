#ifndef __DEBUG_H
#define __DEBUG_H

#include "console.h"
#include "stdarg.h"
#include "elf.h"

//内核调试信息输出 为 1 则输出到串口
#define DEBUG_ALL_SPRINTK 1

//内核信号量调试信息输出 为 1 则输出到串口
#define DEBUG_SEM_SPRINTK 0





//内核调试函数

#define panic(str) _panic(str,__FILE__)

#define assert(x, info)\
	do {\
		if (!(x)) {\
			panic(info);\
		}\
	} while (0)

// 编译期间静态检测
#define static_assert(x)                                	\
	switch (x) { case 0: case (x): ; }

void initKernelELF_Info();
void printk(const char* format, ...);
void printkColor(const char* format, TEXT_color_t back, TEXT_color_t fore, ...);
void sprintk(const char *format, ...);
void printSegStatus();
void _panic(const char* msg, const char* filename);
void printStackTrace();
void printKernelMemStauts();

#endif