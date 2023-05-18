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

//  /fs0/dir01
//  0123456789
//  s   i
// 此函数可能会截断path!
// 解析路径
static int vdev_parse_path(char *path, char **subpath,
                           INode_t **vdev_inode_out) {
    sprintk(" parse:%s in 0x%08X\n", path, path);
    int path_len = strlen(path);

    int i, slash = -1;
    for (i = 0; path[i] != '\0'; i++) {
        if (path[i] == '/') {
            slash = i;
            break;
        }
    }
    // 第一个不是斜杠，是相对路径
    if (slash != 0) {
        *subpath = path;
        return vfs_get_cur_inode(vdev_inode_out);
    }
    // 第一个是斜杠，是绝对路径
    if (slash == 0) {
        i++;
        //sprintk("  i=%d,c=%c,%d\n", i, path[i], path[i]);
        //跳到下一个 '/'
        while (path[i] != '/' && path[i] != '\0') {
            //sprintk("  i=%d,c=%c,%d", i, path[i], path[i]);
            i++;
        }
        // /替换为 '\0'
        path[i] = '\0';
        char *vdev_name = &path[1];
        //避免越界
        if (i < path_len)
            i++;
        *subpath = path + i;

        *vdev_inode_out = vdev_get_root_inode(vdev_name);
        if (*vdev_inode_out == NULL) { // 未找到vdev
            return VDEV_NOT_EXIST;
        }
        return 0;
    }
}
/*
*   warn: 传入的path如果是绝对路径,则此函数会截断path！
*   此函数执行后需补上path的空缺
*/
int vfs_lookup(char *path, INode_t **inode_out) {
    int ret;
    INode_t *vdev_inode = NULL;
    if ((ret = vdev_parse_path(path, &path, &vdev_inode)) != 0) {
        return ret;
    }

    if (*path != '\0') {
        sprintk("\nlookup path:%s\n", path);
        if ((vdev_inode->ops->iop_lookup == NULL)) {
            return -INODE_OP_NULL;
        }
        ret = vdev_inode->ops->iop_lookup(vdev_inode, path, inode_out);
        inode_ref_dec(vdev_inode);
        return ret;
    }
    *inode_out = vdev_inode;
    return 0;
}

int vfs_lookup_parent(char *path, INode_t **vdev_inode_out, char **endp) {
    int ret;
    INode_t *inode = NULL;
    sprintk("path1:%s\n", path);
    if ((ret = vdev_parse_path(path, &path, &inode)) != 0) {
        return ret;
    }
    sprintk("path2:%s\n", path);
    *endp = path;
    *vdev_inode_out = inode;
    return 0;
}

void test_parse_path() {
    sprintk("Test parse vdev:\n");
    INode_t *inode = NULL;
    char *str = "/fs0/abc";
    // sprintk("str:0x%08X\n",str);
    char *out = NULL;
    vdev_parse_path(str, &out, &inode);
    sprintk(" substr:%s\n", out);
    sprintk(" substr in 0x%08X\n", out);
    sprintk(" inode:0x%08X\n", inode);

    sprintk("\n\n");
}
void test_lookup() {
    sprintk("Test lookup:\n");
    INode_t *inode = NULL;
    char *str = "/stdin";

    int ret = vfs_lookup(str, &inode);
    sprintk(" ret:%d inode:0x%08X\n", ret, inode);
    if (inode != NULL) {
        printFatINode(inode);
    }

    sprintk("\n\n");
}
void test_lookup_parent() {
    sprintk("Test lookup parent:\n");
    INode_t *inode = NULL;
    char *str = "/fs0/dir";
    char *out = NULL;
    int ret = vfs_lookup_parent(str, &inode, &out);
    sprintk(" ret:%d inode:0x%08X\n", ret, inode);

    if (out != NULL) {
        sprintk(" out:%s\n", out);
    }
    if (inode != NULL) {
        printFatINode(inode);
    }

    sprintk("\n\n");
}
void test_vfslookup() {
    sprintk("\nTEST VFS LOOKUP:\n");
    // test_parse_path();
    test_lookup();
    test_lookup_parent();
}
