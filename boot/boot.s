; ----------------------------------------------------------------
; 	boot.s -- 内核从这里开始
;   参考：https://www.gnu.org/software/grub/manual/multiboot/multiboot.html
;		  https://blog.csdn.net/weixin_45206746/article/details/113065721
; ----------------------------------------------------------------

MBOOT_HEADER_MAGIC 	equ 	0x1BADB002 	; Multiboot 魔数，由规范决定

MBOOT_PAGE_ALIGN 	equ 	1 << 0    	; 0 号位表示所有的引导模块将按页(4KB)边界对齐
MBOOT_MEM_INFO 		equ 	1 << 1    	; 1 号位表示将内存空间的信息(mem_*)包含在 Multiboot 信息结构中
										; (告诉GRUB把内存空间的信息包含在Multiboot信息结构中)
; 定义 Multiboot 的标记
MBOOT_HEADER_FLAGS 	equ 	MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO

; 域checksum是一个32位的无符号值，要求 magic + flags + checksum = 0
MBOOT_CHECKSUM 		equ 	- (MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)

; 符合Multiboot规范的 OS 映象需要这样一个 Multiboot 头
; Multiboot 头的分布必须如下表所示：
; ----------------------------------------------------------
; 偏移量  类型  域名        备注
;   0     u32   magic       必需
;   4     u32   flags       必需 
;   8     u32   checksum    必需 
;-----------------------------------------------------------

[BITS 32]  	; 所有代码以 32-bit 的方式编译
section .temp.text 	; 代码段从这里开始

; 在代码段的起始位置设置符合 Multiboot 规范的标记
dd MBOOT_HEADER_MAGIC 	; GRUB 魔数，必要
dd MBOOT_HEADER_FLAGS   ; GRUB 的一些加载时选项，其含义在定义处
dd MBOOT_CHECKSUM       ; 检测数值，其含义在定义处

[GLOBAL start] 		; 内核代码入口，提供该声明给 ld 链接器 (script/kernel.ld)
[GLOBAL TempMbootPtr] 	; 临时的 struct multiboot * 变量 (include/multiboot.h)
[EXTERN kernel_entry] 	; 声明内核 C 代码的入口函数 (init/entry.c)

; 引导时，MultiBoot信息结构指针将被保存在ebx中
start:						 ; 内核入口
	cli  			 		 ; 此时还没有设置好中断处理，须关闭中断
	mov [TempMbootPtr], ebx   ;将 ebx 中存储的指针存入 (include/multiboot.h)
	mov esp, STACK_TOP  	 ; 设置内核栈地址
	and esp, 0FFFFFFF0H	 	 ; 栈地址16字节对齐
	mov ebp, 0 		 		 ; 帧指针修改为 0
	call kernel_entry		 ; 调用内核入口函数
stop:
	hlt 			 ; 停机
	jmp stop 		 ; 到这里结束

section .temp.data 	 ; 未初始化的数据段从这里开始
stack:
	resb 1024 	 	 ; 这里作为临时内核栈
TempMbootPtr: 		 ; 临时的 multiboot 结构体指针
	resb 4

STACK_TOP equ $-stack-1 	 ; 内核栈顶，$指当前地址
