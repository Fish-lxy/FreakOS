/*
 *      kernel.ld -- 针对 kernel 的链接脚本
 */
ENTRY(start)/* 设定内核入口，定义在/boot/boot.s */
SECTIONS
{
    /* 段起始位置 */

    PROVIDE(kern_start = 0xC0100000);/* 由链接器提供内核的起始地址(/include/pmm.h) */
    . = 0x100000;
    .temp.text :
    {
        *(.temp.text)
        . = ALIGN(4096);
    }
    .temp.data :
    {
        *(.temp.data)
        . = ALIGN(4096);
    }
    

    . += 0xC0000000;/*设置虚拟地址*/
    .text : AT( ADDR(.text) - 0xC0000000)
    {
        *(.text)
        . = ALIGN(4096);
    }

    .data : AT( ADDR(.data) - 0xC0000000)
    {
        *(.data)
        *(.rodata)
        . = ALIGN(4096);
    }

    .bss : AT( ADDR(.bss) - 0xC0000000)
    {
        *(.bss)
        . = ALIGN(4096);
    }

    .stab : AT( ADDR(.stab) - 0xC0000000)
    {
        *(.stab)
        . = ALIGN(4096);
    }
    
    .symtab : AT( ADDR(.symtab) - 0xC0000000)
    {
        *(.symtab)
        . = ALIGN(4096);
    }
    .stabstr : AT( ADDR(.stabstr) - 0xC0000000)
    {
        *(.stabstr)
        . = ALIGN(4096);
    }
    PROVIDE(kern_end = .);

    /DISCARD/ : { *(.comment) *(.eh_frame) }
}