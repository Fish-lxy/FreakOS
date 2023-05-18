#include "debug.h"
#include "fat.h"
#include "fat_base.h"
#include "inode.h"
#include "kmalloc.h"
#include "list.h"
#include "partiton.h"
#include "sem.h"
#include "task.h"
#include "types.h"
#include "vdev.h"
#include "vfs.h"

static INode_t *get_cur_pwd_inode() { return CurrentTask->files->pwd; }
static void set_cur_pwd_inode(INode_t *inode) {
    CurrentTask->files->pwd = inode;
}
static void lock_cur_files() { lock_files(CurrentTask->files); }
static void unlock_cur_files() { unlock_files(CurrentTask->files); }


int vfs_get_cur_inode(INode_t **inode_out) {
    INode_t *inode;
    if ((inode = get_cur_pwd_inode()) != NULL) {
        inode_ref_inc(inode);
        *inode_out = inode;
        return 0;
    }
    return 1;
}
int vfs_set_cur_inode(INode_t *inode) {
    int ret = 0;
    lock_cur_files();
    INode_t *old_inode;
    if ((old_inode = get_cur_pwd_inode()) != inode) {

        if (inode != NULL) {
            uint32_t type;
            if (inode->ops->iop_get_type == NULL) {
                ret = -INODE_OP_NULL;
                goto out;
            }
            if ((ret = inode->ops->iop_get_type(inode, &type)) != INODE_OK) {
                goto out;
            }
            if (type != INODE_TYPE_DIR) {
                ret = -INDOE_NOT_DIR;
                goto out;
            }
            inode_ref_inc(inode);
            set_cur_pwd_inode(inode);
            if (old_inode != NULL) {
                inode_ref_dec(old_inode);
            }
        }
    }
out:
    unlock_cur_files();
    return ret;
}

int vfs_change_cur_pwd(char* path){
    int ret ;
    INode_t*inode;
    if((ret = vfs_lookup(path,&inode)) == 0){
        ret = vfs_set_cur_inode(inode);
        inode_ref_dec(inode);
    }
    return ret;
}