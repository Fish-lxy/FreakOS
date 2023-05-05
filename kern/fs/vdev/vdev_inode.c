#include "dev_inode.h"
#include "inode.h"

INode_t *create_vdev_inode(INodeOps_t *ops) {
    INode_t *inode = NULL;
    if ((inode = inode_alloc(Dev_Type)) == NULL) {
        return NULL;
    }
    inode_init(inode, ops, NULL);
    return inode;
}
