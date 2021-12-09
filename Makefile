#!Makefile

C_SOURCES = $(shell find . -name "*.c")
C_OBJECTS = $(patsubst %.c, %.o, $(C_SOURCES))
S_SOURCES = $(shell find . -name "*.s")
S_OBJECTS = $(patsubst %.s, %.o, $(S_SOURCES))

CC = gcc
LD = ld
ASM = nasm

C_FLAGS = -c -std=gnu99 -Wall -O1 -m32 -g3 -ggdb -gstabs+ -nostdinc -fno-pic -fno-builtin -fno-stack-protector -I include
LD_FLAGS = -T scripts/kernel.ld -m elf_i386 -nostdlib
ASM_FLAGS = -f elf -g -F stabs

all: $(S_OBJECTS) $(C_OBJECTS) link update_image

.c.o:
	@echo 编译代码文件 $< ...
	$(CC) $(C_FLAGS) $< -o $@

.s.o:
	@echo 编译汇编文件 $< ...
	$(ASM) $(ASM_FLAGS) $<

link:
	@echo 链接内核文件...
	$(LD) $(LD_FLAGS) $(S_OBJECTS) $(C_OBJECTS) -o kernel

.PHONY:clean
clean:
	$(RM) $(S_OBJECTS) $(C_OBJECTS) kernel

.PHONY:update_image
update_image:
	sudo mount disk.img /mnt/kernel
	sudo cp kernel /mnt/kernel/kernel
	sleep 1
	sudo umount /mnt/kernel

.PHONY:mount_image
mount_image:
	sudo mount disk.img /mnt/kernel

.PHONY:umount_image
umount_image:
	sudo umount /mnt/kernel

.PHONY:run
run:
	qemu-system-x86_64 -m 4m -hda disk.img -boot c


.PHONY:debug
debug:
	qemu-system-x86_64 -S -s -hda disk.img -boot c &
	sleep 1
	cgdb -x tools/gdbinit
