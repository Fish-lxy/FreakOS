#include <cpu.h>
#include <debug.h>
#include <idt.h>
#include <intr_sync.h>
#include <kbd.h>
#include <types.h>

void kbdCallBack() {

    int c = kbdGetChar();
    printk("%c", c);
}

void initKBD() {
    enableIRQ(1);
    interruptHandlerRegister(IRQ1, kbdCallBack);
}

int kbdGetChar(void) {
    static uint32_t shift;
    static uint8_t *charcode[4] = {normalmap, shiftmap, ctlmap, ctlmap};
    uint32_t stat, data, c;

    stat = inb(KBSTATP);
    if ((stat & KBS_DIB) == 0)
        return -1;
    data = inb(KBDATAP);

    if (data == 0xE0) {
        shift |= E0ESC;
        return 0;
    } else if (data & 0x80) {
        // 按键抬起
        data = (shift & E0ESC ? data : data & 0x7F);
        shift &= ~(shiftcode[data] | E0ESC);
        return 0;
    } else if (shift & E0ESC) {
        // Last character was an E0 escape; or with 0x80
        data |= 0x80;
        shift &= ~E0ESC;
    }

    shift |= shiftcode[data];
    shift ^= togglecode[data];
    c = charcode[shift & (CTL | SHIFT)][data];
    if (shift & CAPSLOCK) {
        if ('a' <= c && c <= 'z')
            c += 'A' - 'a';
        else if ('A' <= c && c <= 'Z')
            c += 'a' - 'A';
    }
    return c;
}
