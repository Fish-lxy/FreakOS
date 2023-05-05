#include "debug.h"
#include "dev_inode.h"
#include "inode.h"
#include "types.h"
#include "vdev.h"

static const INodeOps_t stdout_dev_op;

void stdout_open(INode_t *inode, int flag) { return 0; }
void stdout_io(INode_t *inode, uint32_t secno, uint32_t nsec, char *buffer,
               uint32_t blen, bool write) {
    if (inode == NULL || buffer == NULL) {
        return INODE_PNULL;
    }
    if (inode->inode_type != Dev_Type) {
        return INODE_NOT_DEV;
    }
    // secno,nsec被忽略
    if (write != FALSE) {
        for (int i = 0; i < blen; i++) {
            consolePutc(buffer[i]);
        }
        return INODE_OK;
    }

    return INODE_OK;
}
int stdout_ioctl(INode_t *inode, int ctrl) { return 0; }

static INodeOps_t *get_stdout_dev_op() { return &stdout_dev_op; }

static void stdout_devinode_init(INode_t *inode) {
    if (inode == NULL) {
        return INODE_PNULL;
    }
    DevINode_t *devinode = &inode->devinode;
    devinode->dev_type = CharDevice;
    devinode->blocks = 0;
    devinode->blocksize = 0;
    return 0;
}

void stdout_init_vdev() {
    INode_t *inode = create_vdev_inode(get_stdout_dev_op());
    stdout_devinode_init(inode);
    vdev_add_dev("stdout", inode, FALSE);
    sprintk("stdout init vedv ok\n");
}

static const INodeOps_t stdout_dev_op = {
    .inode_magic = INODE_OPS_MAGIC,
    .iop_lookup = NULL,
    .iop_open = stdout_open,
    .iop_read = NULL,
    .iop_write = NULL,
    .iop_lseek = NULL,
    .iop_truncate = NULL,
    .iop_create = NULL,
    .iop_readdir = NULL,
    .iop_mkdir = NULL,
    .iop_dev_io = stdout_io,
    .iop_dev_ioctl = stdout_ioctl,
    .iop_release = NULL,
    .iop_delete = NULL,
    .iop_rename = NULL,
    .iop_stat = NULL,
    .iop_sync = NULL,
    .iop_get_type = NULL,
};