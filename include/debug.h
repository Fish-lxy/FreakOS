#ifndef DEBUG_H
#define DEBUG_H

#include "console.h"
#include "stdarg.h"
#include "elf.h"
//内核调试函数

#define assert(x, info)\
	do {\
		if (!(x)) {\
			panic(info);\
		}\
	} while (0)

// 编译期间静态检测
#define static_assert(x)                                	\
	switch (x) { case 0: case (x): ; }

void initDebug();
void printk(const char* format, ...);
void printkColor(const char* format, TEXT_color_t back, TEXT_color_t fore, ...);
void printSegStatus();
void panic(const char* msg);
void printStackTrace();
void printKernelMemStauts();

#endif