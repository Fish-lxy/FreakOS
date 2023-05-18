#include "kmalloc.h"
#include <cpu.h>
#include <debug.h>
#include <idt.h>
#include <intr_sync.h>
#include <kbd.h>
#include <types.h>

void kbdIntrCallBack();

extern void _stdin_do_write(char c);

static KeyBoardBuffer_t *KBDbuffer;

void initKBD() {
    KBDbuffer = (KeyBoardBuffer_t *)kmalloc(sizeof(KeyBoardBuffer_t));

    if (KBDbuffer == NULL) {
        panic("kbd buffer kmalloc error!");
    }
    KBDbuffer->rpos = 0;
    KBDbuffer->wpos = 0;

    enableIRQ(IRQ1);
    intrHandlerRegister(IRQ1, kbdIntrCallBack);
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

    // Process special keys
    // Ctrl-Alt-Del: reboot
    if (!(~shift & (CTL | ALT)) && c == KEY_DEL) {
        sprintk("Ctrl-Alt-Del : Rebooting!\n");
        outb(0x92, 0x3); // courtesy of Chris Frost
    }
    return c;
}

void kbdPutChar2Buffer() {
    int c;
    while ((c = kbdGetChar()) != -1) {
        if (c != 0) {
            KBDbuffer->buffer[KBDbuffer->wpos++] = c;
            if (KBDbuffer->wpos >= KBD_BUFF_LEN) {
                KBDbuffer->wpos = 0;
            }
        }
    }
}
int kbdGetCharFromBuffer() {
    int c = 0;
    bool intr_flag;
    intr_save(intr_flag);
    {
        kbdPutChar2Buffer();

        if (KBDbuffer->rpos != KBDbuffer->wpos) {
            c = KBDbuffer->buffer[KBDbuffer->rpos++];
            if (KBDbuffer->rpos >= KBD_BUFF_LEN) {
                KBDbuffer->rpos = 0;
            }
        }
    }
    intr_restore(intr_flag);
    return c;
}

void kbdIntrCallBack() {

    int c = kbdGetCharFromBuffer();
    _stdin_do_write(c);
    //printk("%c", c);
}

