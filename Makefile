all: kernel 


.PHONY:kernel
kernel:
	@echo "\033[32mComplie Kernel: \033[0m"
	$(MAKE) -C kern
	@echo "\033[32mComplie Kernel OK. \033[0m"

.PHONY:user
user:
	@echo "\033[32mComplie User Program: \033[0m"
	$(MAKE) -C user
	@echo "\033[32mComplie User Program OK. \033[0m"


.PHONY:clean
clean:
	cd kern && $(MAKE) clean
	
.PHONY:mount
mount:
	sudo losetup /dev/loop1 disk.img -o 1048576
	sudo mount /dev/loop1 /mnt/kernel

.PHONY:umount
umount:
	sudo umount /mnt/kernel
	sudo losetup -d /dev/loop1


.PHONY:run
run:
	qemu-system-x86_64 -m 32m -drive format=raw,readonly=off,file=disk.img -boot c -serial stdio

.PHONY:run2
run2:
	qemu-system-x86_64 -m 32m -hda disk.img -hdb disk2.img -boot c -serial stdio


.PHONY:debug
debug:
	qemu-system-x86_64 -S -s -m 32m -hda disk.img -boot c &
	sleep 1
	cgdb -x scripts/gdbinit