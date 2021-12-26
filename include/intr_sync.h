#ifndef INTR_SYNC_H
#define INTR_SYNC_H

#include "types.h"
#include "cpu.h"
#include "idt.h"
#include "mm.h"


//若允许中断，则屏蔽中断
//若中断已被屏蔽，则不做行动
//用于同步中断状态，在需要屏蔽中断的代码段将中断屏蔽，并在代码结束时将恢复原来中断位

//保存中断位
static inline bool
__intr_save(void) {
    if (read_eflags() & FL_IF) {
        cli();
        return 1;
    }
    return 0;
}

//恢复中断位
static inline void
__intr_restore(bool flag) {
    if (flag) {
        sti();
    }
}

#define intr_save(x)      do { x = __intr_save(); } while (0)
#define intr_restore(x)   __intr_restore(x);

#endif
