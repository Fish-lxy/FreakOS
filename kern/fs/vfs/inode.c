#include "inode.h"
#include "debug.h"
#include "kmalloc.h"

void inode_ref_inc(INode_t *inode) { inode->ref++; }
void inode_ref_dec(INode_t *inode) { inode->ref--; }
INode_t *inode_alloc(INodeType_e type) {
    INode_t *inode = NULL;
    if ((inode = kmalloc(sizeof(INode_t))) != NULL) {
        inode->inode_type = type;
        return inode;
    }
    return NULL;
}
void inode_free(INode_t *inode) {
    if (inode != NULL) {
        kfree(inode, sizeof(INode_t));
    }
}
void inode_init(INode_t *inode, INodeOps_t *ops, FileSystem_t *fs) {
    inode->open = 0;
    inode->ref = 0;
    inode->ops = ops;
    inode->fs = fs;
    inode_ref_inc(inode);
}
