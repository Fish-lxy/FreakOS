#include <ide.h>
#include <block_dev.h>
#include <debug.h>
#include <types.h>
#include <cpu.h>

IDEchannel_t ide_channels[2] = {
    {IO_BASE0, IO_CTRL0},
    {IO_BASE1, IO_CTRL1},
};
IDEdevice ide_devices[MAX_IDE];

static int ide_wait_ready(unsigned short iobase, bool check_error);
static int ide_read_secs(unsigned short ideno, uint32_t secno, void* dst, size_t nsecs);
static int ide_write_secs(uint16_t ideno, uint32_t secno, const void* src, size_t nsecs);

static int ide_wait_ready(unsigned short iobase, bool check_error) {
    int r;
    while ((r = inb(iobase + ISA_STATUS)) & IDE_BSY) {}
    if (check_error && (r & (IDE_DF | IDE_ERR)) != 0) {
        return -1;
    }
    return 0;
}
static int ide_read_secs(uint16_t ideno, uint32_t secno, void* dst, size_t nsecs) {
    assert(nsecs <= MAX_NSECS && VALID_IDE(ideno), "nsecs is invalid!");
    assert(secno <= MAX_DISK_NSECS && secno + nsecs <= MAX_DISK_NSECS, "secno is invalid!");
    uint16_t iobase = IO_BASE(ideno);
    uint16_t ioctrl = IO_CTRL(ideno);

    ide_wait_ready(iobase, 0);

    outb(ioctrl + ISA_CTRL, 0);
    outb(iobase + ISA_SECCNT, nsecs);
    outb(iobase + ISA_SECTOR, secno & 0xFF);
    outb(iobase + ISA_CYL_LO, (secno >> 8) & 0xFF);
    outb(iobase + ISA_CYL_HI, (secno >> 16) & 0xFF);
    outb(iobase + ISA_SDH, 0xE0 | ((ideno & 1) << 4) | ((secno >> 24) & 0xF));
    outb(iobase + ISA_COMMAND, IDE_CMD_READ);

    int ret = 0;
    for (;nsecs > 0;nsecs--, dst += SECTSIZE) {
        if ((ret = ide_wait_ready(iobase, 1)) != 0) {
            goto out;
        }
        insl(iobase, dst, SECTSIZE / sizeof(uint32_t));
    }
out:
    return ret;
}
static int ide_write_secs(uint16_t ideno, uint32_t secno, const void* src, size_t nsecs) {
    assert(nsecs <= MAX_NSECS && VALID_IDE(ideno), "nsecs is invalid!");
    assert(secno < MAX_DISK_NSECS&& secno + nsecs <= MAX_DISK_NSECS, "secno is invalid!");
    uint16_t iobase = IO_BASE(ideno), ioctrl = IO_CTRL(ideno);

    ide_wait_ready(iobase, 0);

    outb(ioctrl + ISA_CTRL, 0);
    outb(iobase + ISA_SECCNT, nsecs);
    outb(iobase + ISA_SECTOR, secno & 0xFF);
    outb(iobase + ISA_CYL_LO, (secno >> 8) & 0xFF);
    outb(iobase + ISA_CYL_HI, (secno >> 16) & 0xFF);
    outb(iobase + ISA_SDH, 0xE0 | ((ideno & 1) << 4) | ((secno >> 24) & 0xF));
    outb(iobase + ISA_COMMAND, IDE_CMD_WRITE);

    int ret = 0;
    for (; nsecs > 0; nsecs--, src += SECTSIZE) {
        if ((ret = ide_wait_ready(iobase, 1)) != 0) {
            goto out;
        }
        outsl(iobase, src, SECTSIZE / sizeof(uint32_t));
    }
out:
    return ret;
}

int _ide_request(uint32_t ideno, IOrequest_t* req) {
    if (ideno < 0 || ideno > 3) {
        return -1;
    }
    if (_ide_device_isvaild(ideno) != TRUE) {
        return -1;
    }
    if (req->io_type == IO_READ) {
        if (req->bsize < SECTSIZE * req->nsecs) {
            return -1;
        }
        return ide_read_secs(ideno, req->secno, req->buffer, req->nsecs);
    }
    else if (req->io_type == IO_WRITE) {
        if (req->bsize < SECTSIZE * req->nsecs) {
            return -1;
        }
        return ide_write_secs(ideno, req->secno, req->buffer, req->nsecs);
    }
    return -1;
}
bool _ide_device_isvaild(uint32_t ideno) {
    return ide_devices[ideno].valid == 1;
}
uint32_t _ide_get_nr_block(uint32_t ideno) {
    if (_ide_device_isvaild(ideno)) {
        return ide_devices[ideno].size;
    }
    return 0;
}
const char* _ide_get_desc(uint32_t ideno) {
    return (const char*) (ide_devices[ideno].desc);
}
int _ide_ioctl(uint32_t ideno, int op, int flag) {
    if (op != 0 && flag != 0) {
        return -1;
    }
    return 0;
}

int _ide_device_init(uint16_t ideno) {
    if (ideno > MAX_IDE - 1)
        return -1;
    uint16_t iobase;
    ide_devices[ideno].valid = 0;
    iobase = IO_BASE(ideno);
    ide_wait_ready(iobase, 0);

    //选择主从盘
    outb(iobase + ISA_SDH, 0xE0 | ((ideno & 1) << 4));
    ide_wait_ready(iobase, 0);

    //发送ATA识别命令
    outb(iobase + ISA_COMMAND, IDE_CMD_IDENTIFY);
    ide_wait_ready(iobase, 0);

    //检查设备是否存在
    if (inb(iobase + ISA_STATUS) == 0 || ide_wait_ready(iobase, 1) != 0) {
        return -1;
    }

    ide_devices[ideno].valid = 1;

    uint32_t buffer[128];
    insl(iobase + ISA_DATA, buffer, sizeof(buffer) / sizeof(uint32_t));

    uint8_t* ident = (uint8_t*) buffer;
    uint32_t sectors;
    uint32_t cmdsets = *(uint32_t*) (ident + IDE_IDENT_CMDSETS);
    if (cmdsets & (1 << 26)) {
        sectors = *(uint32_t*) (ident + IDE_IDENT_MAX_LBA_EXT);
    }
    else {
        sectors = *(uint32_t*) (ident + IDE_IDENT_MAX_LBA);
    }
    ide_devices[ideno].sets = cmdsets;
    ide_devices[ideno].size = sectors;
    //检查LBA
    assert((*(uint16_t*) (ident + IDE_IDENT_CAPABILITIES) & 0x200) != 0, "LBA is not supported!");

    uint8_t* desc = ide_devices[ideno].desc;
    uint8_t* data = ident + IDE_IDENT_MODEL;
    uint32_t i, length = 40;
    for (i = 0;i < length;i += 2) {
        desc[i] = data[i + 1];
        desc[i + 1] = data[i];
    }
    do {
        desc[i] = '\0';
    } while (i-- > 0 && desc[i] == ' ');
    
    return 0;
}
