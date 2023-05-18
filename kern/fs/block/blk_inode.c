#include "bcache.h"
#include "block.h"
#include "dev_inode.h"
#include "inode.h"
#include "vdev.h"
#include "types.h"
#include "debug.h"

extern BlockDevice_t *BlockDeviceList;

static const INodeOps_t blk_dev_op;

void blkdev_open(INode_t *inode, int flag) { return 0; }
void blkdev_io(INode_t *inode, uint32_t secno, uint32_t nsec, char *buffer,
               uint32_t blen, bool write) {
    if (inode == NULL || buffer == NULL) {
        return -INODE_PNULL;
    }
    if (inode->inode_type != Dev_Type) {
        return -INODE_NOT_DEV;
    }
    int ret;
    BlockDevice_t *dev = inode->devinode.blkdev;
    IOrequest_t req;
    req.io_type = write;
    req.buffer = buffer;
    req.bsize = blen;
    req.secno = secno;
    req.nsecs = nsec;

    if ((ret = diskIO_BCache(dev, &req)) != NULL) {
        return -INODE_INTERNAL_ERR;
    }
    return INODE_OK;
}
int blkdev_ioctl(INode_t *inode, int ctrl) { return 0; }

static INodeOps_t *get_blk_dev_op() { return &blk_dev_op; }

static char *get_new_blk_name() {
    static char name[7] = "block0";
    static int counter = -1;
    counter++;
    name[5] = counter + '0';
    sprintk("getname:%s\n",name);
    return name;
}
static int blkdev_devinode_init(INode_t *inode, BlockDevice_t *blkdev) {
    if (inode == NULL) {
        return -INODE_PNULL;
    }
    DevINode_t *devinode = &inode->devinode;
    devinode->blkdev = blkdev;
    devinode->dev_type = BlockDevice;
    devinode->blocks = blkdev->ops.get_nr_block();
    devinode->blocksize = blkdev->block_size;
    return 0;
}

void blk_init_vdev() {
    list_ptr_t *listi = NULL;
    BlockDevice_t *blockdev = NULL;
    listForEach(listi, &(BlockDeviceList->list_ptr)) {
        blockdev = lp2blockdev(listi, list_ptr);
        if (blockdev != NULL) {
            INode_t *inode = create_vdev_inode(get_blk_dev_op());
            blkdev_devinode_init(inode, blockdev);
            vdev_add_dev(get_new_blk_name(), inode, TRUE);
            sprintk("blk init vedv ok\n");
        }
    }
}

// 块设备 INode 操作数组
static const INodeOps_t blk_dev_op = {
    .inode_magic = INODE_OPS_MAGIC,
    .iop_lookup = NULL,
    .iop_open = blkdev_open,
    .iop_read = NULL,
    .iop_write = NULL,
    .iop_lseek = NULL,
    .iop_truncate = NULL,
    .iop_create = NULL,
    .iop_readdir = NULL,
    .iop_mkdir = NULL,
    .iop_dev_io = blkdev_io,
    .iop_dev_ioctl = blkdev_ioctl,
    .iop_release = NULL,
    .iop_delete = NULL,
    .iop_rename = NULL,
    .iop_stat = NULL,
    .iop_sync = NULL,
    .iop_get_type = NULL,
};