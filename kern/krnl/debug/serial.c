#include "serial.h"
#include "cpu.h"
#include "idt.h"
#include "types.h"

static bool SerialExist = FALSE;
/* stupid I/O delay routine necessitated by historical PC design flaws */
static void delay(void) {
    inb(0x84);
    inb(0x84);
    inb(0x84);
    inb(0x84);
}
static void serial_putc_sub(int c) {
    int i;
    for (i = 0; !(inb(COM1 + COM_LSR) & COM_LSR_TXRDY) && i < 12800; i++) {
        delay();
    }
    outb(COM1 + COM_TX, c);
}
/* serial_putc - print character to serial port */
static void serialPutChar(int c) {
    if (c != '\b') {
        serial_putc_sub(c);
    } else {
        serial_putc_sub('\b');
        serial_putc_sub(' ');
        serial_putc_sub('\b');
    }
}

void serialWriteStr(char* str) {
    while (*str) {
        serialPutChar(*str);
        str++;
    }
}

void initDebugSerial() {
    // Turn off the FIFO
    outb(COM1 + COM_FCR, 0);

    // Set speed; requires DLAB latch
    outb(COM1 + COM_LCR, COM_LCR_DLAB);
    outb(COM1 + COM_DLL, (uint8_t)(115200 / 9600));
    outb(COM1 + COM_DLM, 0);

    // 8 data bits, 1 stop bit, parity off; turn off DLAB latch
    outb(COM1 + COM_LCR, COM_LCR_WLEN8 & ~COM_LCR_DLAB);

    // No modem controls
    outb(COM1 + COM_MCR, 0);
    // Enable rcv interrupts
    outb(COM1 + COM_IER, COM_IER_RDI);

    // Clear any preexisting overrun indications and interrupts
    // Serial port doesn't exist if COM_LSR returns 0xFF
    SerialExist = (inb(COM1 + COM_LSR) != 0xFF);
    (void)inb(COM1 + COM_IIR);
    (void)inb(COM1 + COM_RX);

    if (SerialExist) {
        enableIRQ(IRQ4);
    }
    printk("Serial port for debug:%s\n", SerialExist ? "Yes" : "No");
    
}
