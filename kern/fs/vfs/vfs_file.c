#include "debug.h"
#include "fat.h"
#include "fat_base.h"
#include "inode.h"
#include "kmalloc.h"
#include "list.h"
#include "partiton.h"
#include "sem.h"
#include "string.h"
#include "task.h"
#include "types.h"
#include "vdev.h"
#include "vfs.h"

int vfs_open(char *path, uint32_t open_flag, INode_t **inode_out) {
    int path_len = strlen(path);
    char *path_in = kmalloc(sizeof(char) * path_len);
    if (path_in == NULL) {
        return VFS_MEM_ERR;
    }
    strcpy(path_in, path);

    bool can_write = 0;
    switch (open_flag & OPEN_MASK) {
    case OPEN_READ:
        break;
    case OPEN_WRITE:
    case OPEN_RW:
        can_write = TRUE;
        break;
        // default:
        // return VFS_INVAL;
    }

    bool exist = (open_flag & OPEN_EXIST) != 0;
    bool create = (open_flag & OPEN_CREATE) != 0;

    int ret;
    INode_t *inode = NULL;
    ret = vfs_lookup(path_in, &inode);// path_in 被 '\0' 截断
    // 补上空缺
    for (int i = 0; i < path_len; i++) {
        if (path_in[i] == '\0') {
            path_in[i] = '/';
        }
    }

    sprintk("path:%s\n", path_in);

    // 未找到，文件不存在
    if (ret != 0) {
        // 创建文件
        if ((create)) {
            char *name;
            INode_t *dir;
            // 找到路径所属dir inode
            if ((ret = vfs_lookup_parent(path_in, &dir, &name))) {
                ret = VFS_INVAL;
                goto out;
            }
            sprintk("  name:%s\n", name);

            if (dir->ops->iop_create == NULL) {
                sprintk("  create op inval!");
                ret = VFS_INVAL_OP;
                goto out;
            }
            if ((ret = dir->ops->iop_create(dir, name, INODE_CREATE_EXIST)) !=
                0) {
                sprintk("  create op failed!");
                ret = VFS_OP_FAILED;
                goto out;
            }

            // 补上空缺
            for (int i = 0; i < path_len; i++) {
                if (path_in[i] == '\0') {
                    path_in[i] = '/';
                }
            }
            sprintk("path_in:%s\n", path_in);
            ret = vfs_lookup(path_in, &inode);
            if (ret != 0) {
                sprintk("create err\n");
            }
        } else {
            ret = VFS_NOT_EXIST;
            goto out;
        }
    } else if (exist && create) {
        ret = VFS_EXIST;
        goto out;
    }

    if (inode == NULL) {
        ret = VFS_PNULL;
        goto out;
    }

    if (inode->ops->iop_open == NULL) {
        sprintk("  open op inval!");
        ret = VFS_INVAL_OP;
        goto out;
    }
    if ((ret = inode->ops->iop_open(inode, open_flag)) != 0) {
        int m = inode->ops->inode_magic;
        // sprintk(" inode :0x%08X m=%08X ret=%d open op failed!",inode,m,ret);
        inode_ref_dec(inode);
        goto out;
    }

    inode_open_inc(inode);
    if (open_flag & OPEN_TRUNC || create) {
        if (inode->ops->iop_truncate == NULL) {
            sprintk("  truncate op inval!");
            ret = VFS_INVAL_OP;
            goto out;
        }
        if ((ret = inode->ops->iop_truncate(inode, 0)) != 0) {
            sprintk("  truncate op failed!");
            inode_ref_dec(inode);
            inode_open_dec(inode);
            ret = VFS_OP_FAILED;
            goto out;
        }
    }

    *inode_out = inode;
    ret = VFS_OK;

out:
    kfree(path_in, sizeof(char) * path_len);
    return ret;
}
int vfs_close(INode_t *inode) {
    int ret;
    if (inode == NULL) {
        ret = VFS_INVAL;
        return ret;
    }
    if (inode->ops->iop_sync == NULL) {
        sprintk("  close: sync op inval!");
        ret = VFS_INVAL_OP;
        return ret;
    }
    if ((ret = inode->ops->iop_sync(inode)) != 0) {
        sprintk("  close: sync op failed!");
        inode_ref_dec(inode);
        inode_open_dec(inode);
        return VFS_OP_FAILED;
    }
    inode_open_dec(inode);
    inode_ref_dec(inode);
    ret = 0;
    return ret;
}

void test_vfs_open() {
    INode_t *inode = NULL;
    char *path = "/stdin";
    int ret = vfs_open(path, OPEN_RW , &inode);

    sprintk(" open ret:%d inode:0x%08X\n", ret, inode);
    if (inode != NULL) {
        printFatINode(inode);
    }

    ret = vfs_close(inode);
    sprintk(" close ret:%d\n", ret);
}
void test_vfs_file() {
    sprintk("\nTEST VFS FILE:\n");
    test_vfs_open();
}