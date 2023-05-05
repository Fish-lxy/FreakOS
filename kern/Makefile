#!Makefile

C_SOURCES = $(shell find . -name "*.c")
C_OBJECTS = $(patsubst %.c, %.o, $(C_SOURCES))
S_SOURCES = $(shell find . -name "*.s")
S_OBJECTS = $(patsubst %.s, %.o, $(S_SOURCES))

CC = gcc
LD = ld
ASM = nasm

C_FLAGS = -c -O0 -std=gnu99 -Wall -m32 -ggdb -gstabs+ -nostdinc -fno-pic -fno-stack-protector -I include
LD_FLAGS = -T scripts/kernel.ld -m elf_i386 -nostdlib
ASM_FLAGS = -f elf -g -F stabs

VPATH = ./

all: $(S_OBJECTS) $(C_OBJECTS) link update

.c.o:
	@echo Complie $< ...
	$(CC) $(C_FLAGS) $< -o $@

.s.o:
	@echo Complie $< ...
	$(ASM) $(ASM_FLAGS) $<

link:
	@echo Link...
	$(LD) $(LD_FLAGS) $(S_OBJECTS) $(C_OBJECTS) -o kernel

.PHONY:clean
clean:
	$(RM) $(S_OBJECTS) $(C_OBJECTS) kernel

.PHONY:update
update:
	sudo losetup /dev/loop1 ../disk.img -o 1048576
	sudo mount /dev/loop1 /mnt/kernel
	sudo cp kernel /mnt/kernel/kernel
	sync
	sudo umount /mnt/kernel
	sudo losetup -d /dev/loop1


