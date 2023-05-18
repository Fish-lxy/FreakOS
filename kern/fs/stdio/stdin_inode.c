#include "debug.h"
#include "dev_inode.h"
#include "inode.h"
#include "intr_sync.h"
#include "kmalloc.h"
#include "sched.h"
#include "types.h"
#include "vdev.h"
#include "wait.h"

#define STDIN_BUF_LEN 2048
static char *stdin_buffer;
static int32_t p_rpos, p_wpos;
static Wait_t _wait_queue;
static Wait_t *wait_queue = &_wait_queue;

static const INodeOps_t stdin_dev_op;

void _stdin_do_write(char c) {
    bool intr_flag;
    if (c != '\0') {
        intr_save(intr_flag);
        {
            stdin_buffer[p_wpos % STDIN_BUF_LEN] = c;
            if (p_wpos - p_rpos < STDIN_BUF_LEN) {
                p_wpos++;
            }
            if (waitlistIsEmpty(wait_queue) != TRUE) {
                wakeupAll(wait_queue, WT_KBD, 1);
            }
        }
        intr_restore(intr_flag);
    }
}
static int _stdin_do_read(char *buf, uint32_t len) {
    int ret = 0;
    bool intr_flag;
    intr_save(intr_flag);
    {
        for (; ret < len; ret++, p_rpos++) {
        try_again:
            if (p_rpos < p_wpos) {
                *buf++ = stdin_buffer[p_rpos % STDIN_BUF_LEN];
            } else {
                Wait_t _wait;
                Wait_t *wait = &_wait;
                waitAddCurrent(wait_queue, wait, WT_KBD);
                // sprintk("stdin:当前进程%d开始睡眠\n", CurrentTask->tid);
                intr_restore(intr_flag);

                schedule();

                intr_save(intr_flag);
                waitDelCurrent(wait_queue, wait);
                // sprintk("stdin:进程%d被唤醒\n", CurrentTask->tid);

                if (wait->flags == WT_KBD) {
                    goto try_again;
                }
                break;
            }
        }
    }
    intr_restore(intr_flag);
    return ret;
}

int stdin_open(INode_t *inode, int flag) { return 0; }
int stdin_io(INode_t *inode, uint32_t secno, uint32_t nsec, char *buffer,
             uint32_t blen, bool write) {
    if (inode == NULL || buffer == NULL) {
        return -INODE_PNULL;
    }
    if (inode->inode_type != Dev_Type) {
        return -INODE_NOT_DEV;
    }
    // secno,nsec被忽略
    if (write != TRUE) { // 只能读
        int32_t len = blen;
        int ret;
        if ((ret = _stdin_do_read(buffer, len)) > 0) {
            len -= ret;
        }
        return ret;
    }

    return -INODE_DENIED;
}

int stdin_read(INode_t *inode, char *buffer, uint32_t len,uint32_t copied) {
    return stdin_io(inode, 0, 0, buffer, len, FALSE);
}

int stdin_ioctl(INode_t *inode, int ctrl) { return 0; }

static INodeOps_t *get_stdin_dev_op() { return &stdin_dev_op; }

static void stdin_devinode_init(INode_t *inode) {
    if (inode == NULL) {
        return -INODE_PNULL;
    }
    DevINode_t *devinode = &inode->devinode;
    devinode->dev_type = CharDevice;
    devinode->blocks = 0;
    devinode->blocksize = 0;
    return 0;
}

void stdin_init_vdev() {
    stdin_buffer = (char *)kmalloc(sizeof(char) * 2048);
    if (stdin_buffer == NULL) {
        panic("stdin buffer kmalloc error!");
    }
    waitlistInit(wait_queue);

    INode_t *inode = create_vdev_inode(get_stdin_dev_op());
    stdin_devinode_init(inode);
    vdev_add_dev("stdin", inode, FALSE);
    sprintk("stdin init vedv ok\n");
}

static const INodeOps_t stdin_dev_op = {
    .inode_magic = INODE_OPS_MAGIC,
    .iop_lookup = NULL,
    .iop_open = stdin_open, //
    .iop_read = stdin_read,
    .iop_write = NULL,
    .iop_lseek = NULL,
    .iop_truncate = NULL,
    .iop_create = NULL,
    .iop_readdir = NULL,
    .iop_mkdir = NULL,
    .iop_dev_io = stdin_io,       //
    .iop_dev_ioctl = stdin_ioctl, //
    .iop_release = NULL,
    .iop_delete = NULL,
    .iop_rename = NULL,
    .iop_stat = NULL,
    .iop_sync = NULL,
    .iop_get_type = NULL,
};