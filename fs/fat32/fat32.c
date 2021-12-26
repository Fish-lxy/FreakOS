#include "fat32.h"
#include "mbr.h"
#include "ide.h"
#include "block_dev.h"

#include "types.h"
#include "pmm.h"
#include "debug.h"


extern BlockDev_t main_blockdev;
static void read() {

}
void fat32_test() {
    uint8_t* sec = kmalloc(512);
    uint32_t start = PART_START_SECTOR(0);
    printk("%d\n", start);
    IOrequest_t req = { IO_READ, start, 1, sec, 512 };
    if (main_blockdev.ops.request(&req) != 0) {
        panic("IO error!");
    }
    
    for (int i = 0;i < SECTSIZE / 2;i++) {
        if (i % 16 == 0) {
            printk("\n");
        }
        printk("%02X ", sec[i]);
    }
    for (int i = 0;i < SECTSIZE / 2;i++) {
        if (i % 16 == 0) {
            printk("\n");
        }
        printk("%c", sec[i]);
    }
    printk("\n");
}