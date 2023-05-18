#include "vdev.h"
#include "block.h"
#include "inode.h"
#include "kmalloc.h"
#include "sem.h"
#include "string.h"
#include "vfs.h"

extern BlockDevice_t *BlockDeviceList;
extern FileSystem_t FS_List;

static vDev_t vDevList;
static Semaphore_t vDevListSem;

void init_vDev() {
    initList(&(vDevList.ptr));
    initSem(&vDevListSem, 1, "vDevListSem");

    // 将所有设备加入vDev
    // 添加标准输入输出
    stdin_init_vdev();
    stdout_init_vdev();

    // 遍历并添加磁盘设备
    blk_init_vdev();
    // 添加存在的文件系统
    fs_init_vdev();
}
static void lock_vdevlist() { acquireSem(&vDevListSem); }
static void unlock_vdevlist() { releaseSem(&vDevListSem); }

static void vdev_add_list(vDev_t *vdev) {
    listAdd(&(vDevList.ptr), &(vdev->ptr));
}
INode_t *vdev_get_root_inode(const char *name) {
    vDev_t *vdev = NULL;
    list_ptr_t *i = NULL;
    list_ptr_t *list = &vDevList.ptr;
    listForEach(i, list) {
        vdev = lp2vdev(i, ptr);
        if (strcmp(name, vdev->name) == 0) {
            if (vdev->fs != NULL) {
                FileSystem_t *fs = vdev->fs;
                if (fs->get_root_inode != NULL) {
                    return fs->get_root_inode(fs);
                }
            } else if (vdev->inode != NULL) {
                return vdev->inode;
            }
        }
    }
    return NULL;
}

const char *vdev_get_fs_name(FileSystem_t *fs) {
    vDev_t *vdev = NULL;
    list_ptr_t *i = NULL;
    list_ptr_t *list = &vDevList.ptr;
    listForEach(i, list) {
        vdev = lp2vdev(i, ptr);
        if (vdev->fs == fs) {
            return vdev->name;
        }
    }
    return NULL;
}

static int check_vdev_name(const char *name) {
    vDev_t *vdev = NULL;
    list_ptr_t *i = NULL;
    list_ptr_t *list = &vDevList.ptr;
    listForEach(i, list) {
        vdev = lp2vdev(i, ptr);
        if (strcmp(name, vdev->name) == 0) {
            return 1;
        }
    }
    return 0;
}

static int vdev_add(const char *vdev_name, INode_t *dev_inode, FileSystem_t *fs,
                    bool mountable) {
    if (vdev_name == NULL) {
        return VDEV_PNULL;
    }
    // sprintk("vdev add 1 OK\n");
    // 添加文件系统的情况：dev_inode为空且不可挂载
    // 添加物理设备的情况：dev_inode非空且类型是Device
    if (!((dev_inode == NULL && fs != NULL && mountable != TRUE) ||
          (dev_inode != NULL && dev_inode->inode_type == Dev_Type))) {
        return VDEV_TYPE_ERR;
    }
    // sprintk("vdev add 2 OK\n");
    int ret = 0;
    char *new_dev_name = NULL;
    if ((new_dev_name = kmalloc(VDEV_NAME_LEN)) == NULL) {
        return VDEV_INTERNAL_ERR;
    }
    memset(new_dev_name, 0, VDEV_NAME_LEN);
    strcpy(new_dev_name, vdev_name);

    // sprintk("vdev add 3 OK\n");
    lock_vdevlist();

    if (check_vdev_name(vdev_name) != 0) {
        ret = VDEV_NAME_CHECK_ERR;
        goto failed_name_err;
    }
    // sprintk("vdev add 4 OK\n");

    vDev_t *new_vdev = NULL;
    if ((new_vdev = kmalloc(sizeof(vDev_t))) == NULL) {
        ret = VDEV_INTERNAL_ERR;
        goto failed_alloc_vdev;
    }
    // sprintk("vdev add 5 OK\n");

    new_vdev->name = new_dev_name;
    new_vdev->inode = dev_inode;
    new_vdev->fs = fs;
    new_vdev->mountable = mountable;

    vdev_add_list(new_vdev);
    // sprintk("vdev add 6 OK\n");
    unlock_vdevlist();
    return 0;

failed_alloc_vdev:
failed_name_err:
    unlock_vdevlist();
    kfree(new_dev_name, sizeof(vDev_t));
}
int vdev_add_dev(const char *name, INode_t *inode, bool mountable) {
    return vdev_add(name, inode, NULL, mountable);
}
int vdev_add_fs(const char *name, FileSystem_t *fs) {
    return vdev_add(name, NULL, fs, FALSE);
}





void sprint_vDev(vDev_t *vdev) {
    sprintk("print vdev in 0x%08X:\n", vdev);
    if (vdev == NULL) {
        sprintk(" vdev is null!\n");
        return;
    }
    sprintk(" name:%s\n", vdev->name);
    sprintk(" inode:0x%08X\n", vdev->inode);
    sprintk(" fs:0x%08X\n", vdev->fs);
    sprintk(" mountable:%d\n", vdev->mountable);
}
void test_list_vDev() {
    vDev_t *vdev = NULL;
    list_ptr_t *i = NULL;
    list_ptr_t *list = &vDevList.ptr;
    listForEach(i, list) {
        vdev = lp2vdev(i, ptr);
        if (vdev != NULL) {
            sprint_vDev(vdev);
        }
    }
}
vDev_t *test_lookup_vDev(const char *name) {
    vDev_t *vdev = NULL;
    list_ptr_t *i = NULL;
    list_ptr_t *list = &vDevList.ptr;
    listForEach(i, list) {
        vdev = lp2vdev(i, ptr);
        if (strcmp(name, vdev->name) == 0) {
            return vdev;
        }
    }
    return NULL;
}




void test_putc(char c) {
    vDev_t *stdout = test_lookup_vDev("stdout");
    if (stdout == NULL) {
        sprintk("vdev lookup null!\n");
    }
    INode_t *inode = stdout->inode;
    inode->ops->iop_dev_io(inode, 0, 0, &c, 1, TRUE);
}
char test_getc() {
    vDev_t *stdin = test_lookup_vDev("stdin");
    if (stdin == NULL) {
        sprintk("vdev lookup null!\n");
    }
    char c = 0;
    INode_t *inode = stdin->inode;
    inode->ops->iop_dev_io(inode, 0, 0, &c, 1, FALSE);
    return c;
}

#define READLINE_BUFLEN 1024
void test_vdev_readline(char *out) {
    static char buf[READLINE_BUFLEN];

    int i = 0;
    while (1) {
        char c;
        if ((c = test_getc()) < 0) {
            return NULL;
        } else if (c == 0) {
            if (i > 0) {
                buf[i] = '\0';
                break;
            }
            return NULL;
        }

        if (c == 3) {
            return NULL;
        } else if (c >= ' ' && i < READLINE_BUFLEN - 1) {
            test_putc(c);
            buf[i++] = c;
        } else if (c == '\b' && i > 0) {
            test_putc(c);
            test_putc(' ');
            test_putc(c);
            i--;
        } else if (c == '\n' || c == '\r') {
            test_putc(c);
            buf[i] = '\0';
            break;
        }
    }
    strcpy(out, buf);
}
void test_vDev() {
    sprintk("vDev TEST:\n");
    test_list_vDev();
}