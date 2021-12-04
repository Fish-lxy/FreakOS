#ifndef IO_H
#define IO_H

#include "types.h"

void outb(uint16_t port, uint8_t value); //向端口写一个字节
uint8_t inb(uint16_t port);              //从端口读取一个字节
uint16_t inw(uint16_t port);             //从端口读取一个字

#endif