#include "debug.h"
#include "dev.h"
#include "fat.h"
#include "fat_base.h"
#include "inode.h"
#include "kmalloc.h"
#include "list.h"
#include "partiton.h"
#include "sem.h"
#include "string.h"
#include "types.h"
#include "vfs.h"

int fat_create(INode_t *inode);             // 创建文件
int fat_openfile(INode_t *inode, int flag); // 打开/创建文件
int fat_write(INode_t *inode, void *buf, uint32_t len);
int fat_read(INode_t *inode, void *buf, uint32_t len); // 作为函数指针存入INode

int fat_opendir(INode_t *inode, int flag);

int fat_close(INode_t *inode);
int fat_rename(INode_t *inode, char *new);
int fat_stat(INode_t *inode, Stat_t *stat);
int fat_sync(INode_t *inode);
int fat_lseek(INode_t *inode, int32_t off);

static INodeOps_t *get_fat_inode_ops();
static uint32_t getNewFatInodeId(FatFs_t *fatfs);
static void fat_add_inode_list(FatFs_t *fatfs, FatINode_t *fat_inode);
static void fat_del_inode_list(FatFs_t *fatfs, FatINode_t *fat_inode);
static char *construct_internal_path(FatFs_t *fatfs, char *dest, char *path);
static INode_t *find_list_fat_inode_withID_nolock(FatFs_t *fatfs, uint32_t id);
static INode_t *
find_list_fatinode_with_internal_path_nolock(FatFs_t *fatfs,
                                             const char *inpath);
static INode_t *fat_create_inode(FatFs_t *fatfs);
static INode_t *set_fatinode_info(INode_t *inode, FatFs_t *fatfs,
                                  FATBaseType_e type, const char *inpath);
static INode_t *fat_lookup_inode(FatFs_t *fatfs, char *path);





static INode_t *fat_create_inode2(FatFs_t *fatfs,
                                  FATBase_InodeData_u *base_data,
                                  FATBaseType_e type);
static INode_t *fat_load_inode(FatFs_t *fatfs, uint32_t id);

//------------------------------------------------------------------------------------------
static void fat_add_inode_list(FatFs_t *fatfs, FatINode_t *fat_inode) {
    listAdd(&fatfs->inode_list, &fat_inode->list_ptr);
}
static void fat_del_inode_list(FatFs_t *fatfs, FatINode_t *fat_inode) {
    listDel(&fat_inode->list_ptr);
}
//构造fat驱动所需内部路径
static char *construct_internal_path(FatFs_t *fatfs, char *dest, char *path) {
    if (fatfs == NULL || dest == NULL || path == NULL) {
        return NULL;
    }
    char head[3] = "0:";
    head[0] = fatfs->fatbase_part_index + '0';

    strcat(dest, head);

    if (path[0] != '/') {
        strcat(dest, "/");
    }
    strcat(dest, path);
    return dest;
}

// 实现兼容层read/write
static int fat_rw(FatFs_t *fatfs, FatINode_t *fat_inode, void *buf, uint32_t len,
                  bool write) {}
// 获得一个新的fatinode id
static uint32_t getNewFatInodeId(FatFs_t *fatfs) {
    uint32_t ret = fatfs->next_id;
    fatfs->next_id++;
    return ret;
}

// 通过 fatinode id ，在内存中查找INode_t
static INode_t *find_list_fat_inode_withID_nolock(FatFs_t *fatfs, uint32_t id) {
    list_ptr_t *list = &fatfs->inode_list;
    list_ptr_t *i = NULL;
    FatINode_t *tempi = NULL;
    INode_t *inode = NULL;
    listForEach(i, list) {
        tempi = lp2fatinode(i, list_ptr);
        if (tempi != NULL && tempi->id == id) {
            inode = fatinode2inode(tempi, fatinode);
            return inode;
        }
    }
    return NULL;
}
// 通过内部路径在内存中查找INode_t
static INode_t *
find_list_fatinode_with_internal_path_nolock(FatFs_t *fatfs,
                                             const char *inpath) {
    list_ptr_t *list = &fatfs->inode_list;
    list_ptr_t *i = NULL;
    FatINode_t *tempi = NULL;
    INode_t *inode = NULL;
    listForEach(i, list) {
        tempi = lp2fatinode(i, list_ptr);
        if (strcmp(tempi->internal_path, inpath) == 0) {
            inode = fatinode2inode(tempi, fatinode);
            return inode;
        }
    }
    return NULL;
}
//建立新的inode
static INode_t *fat_create_inode(FatFs_t *fatfs) {
    INode_t *inode = NULL; // in heap
    if ((inode = inode_alloc(FAT_Type)) != NULL) {
        inode_init(inode, get_fat_inode_ops(), fatfs2fs(fatfs, fatfs));
        return inode;
    }
    return NULL;
}
static INode_t *set_fatinode_info(INode_t *inode, FatFs_t *fatfs,
                                  FATBaseType_e type, const char *inpath) {
    if (inode != NULL) {
        FatINode_t *fatinode = &inode->fatinode;
        fatinode->internal_path = inpath;
        fatinode->fatfs = fatfs;
        fatinode->fatinode_type = type;
        fatinode->id = getNewFatInodeId(fatfs);
        initSem(&fatinode->sem, 1);
        return inode;
    }
    return NULL;
}

//  从内存和磁盘查找文件INode，
//  内存存在则返回INode，内存不存在但磁盘存在则创建后返回，文件不存在则返回NULL
static INode_t *fat_lookup_inode(FatFs_t *fatfs, char *path) {
    lockFatFS(fatfs);

    if (fatfs == NULL || path == NULL) {
        goto failed_unlock;
    }
    sprintk("1ok\n");

    // 构造内部路径
    char *internal_path; // in heap
    if ((internal_path = (char *)kmalloc(INTERNAL_PATH_LEN)) == NULL) {
        goto failed_unlock;
    }
    memset(internal_path, 0, INTERNAL_PATH_LEN);
    internal_path = construct_internal_path(fatfs, internal_path, path);
    sprintk("2ok %s\n", internal_path);

    // 在fatfs的inode链表中查找
    INode_t *inode = NULL;
    if ((inode = find_list_fatinode_with_internal_path_nolock(
             fatfs, internal_path)) != NULL) {
        goto failed_inpath;
    }
    sprintk("3ok find inode in list:0x%08X\n", inode);

    // 内存中没有需要的inode

    // 读磁盘 构造inode
    // 申请空inode
    if ((inode = inode_alloc(FAT_Type)) == NULL) {
        goto failed_inpath;
    }
    inode_init(inode, get_fat_inode_ops(), fatfs2fs(fatfs, fatfs));
    sprintk("4ok alloc inode:0x%08X\n", inode);

    // 利用fat底层驱动的f_open，读取FATBase_InodeData
    FATBase_InodeData_u *base_node_data = &(inode->fatinode.fatbase_data);

    FATBaseType_e type;
    if (fatbase_open(&(base_node_data->file), internal_path, FA_READ) ==
        FR_OK) {
        type = FAT_Type_File;
    } else if (fatbase_opendir(&(base_node_data->dir), internal_path) ==
               FR_OK) {
        type = FAT_Type_Dir;
    } else {
        goto failed_f_open;
    }

    if (set_fatinode_info(inode, fatfs, type, internal_path) == NULL) {
        goto failed_f_open;
    }

    fat_add_inode_list(fatfs, &(inode->fatinode.list_ptr));

    sprintk("5ok type:%d \n", type);

out:
    unlockFatFS(fatfs);
    return inode;

failed_f_open:
    inode_free(inode);
failed_inpath:
    kfree(internal_path, INTERNAL_PATH_LEN);
failed_unlock:
    unlockFatFS(fatfs);
    return NULL;
}

static INodeOps_t *get_fat_inode_ops() { return NULL; }


// -------------------------------------------------------------------------------------
// unused

//  通过FATBase_InodeData创建新的内存INode，初始化数据
static INode_t *fat_create_inode2(FatFs_t *fatfs,
                                  FATBase_InodeData_u *base_data,
                                  FATBaseType_e type) {
    INode_t *inode;
    if ((inode = inode_alloc(FAT_Type)) != NULL) {
        inode_init(inode, get_fat_inode_ops(), fatfs2fs(fatfs, fatfs));

        FatINode_t *fatinode = &inode->fatinode;
        fatinode->fatfs = fatfs;
        fatinode->fatinode_type = type;
        memcpy(&fatinode->fatbase_data, base_data, sizeof(FATBase_InodeData_u));
        fatinode->id = getNewFatInodeId(fatfs);
        initSem(&fatinode->sem, 1);
        return inode;
    }
    return NULL;
    // TODO
}

//------------------------------------------------------------------------------------------------
// TEST
//------------------------------------------------------------------------------------------------
static void printFatINode(INode_t *inode) {
    sprintk("print inode:\n");
    if (inode == NULL) {
        sprintk(" inode is null!\n");
        return;
    }
    FatINode_t *fatinode = &inode->fatinode;
    sprintk(" id:%d\n", fatinode->id);
    sprintk(" in path:%s\n", fatinode->internal_path);
    sprintk(" type:%s\n",
            fatinode->fatinode_type == FAT_Type_File ? "file" : "dir");
    if (fatinode->fatinode_type == FAT_Type_File) {
        FATBase_FILE *base = &fatinode->fatbase_data;
        sprintk("  id:%d\n", base->id);         /* Owner file system mount ID */
        sprintk("  flag:0x%08X\n", base->flag); /* File status flags */
        sprintk("  sect_clust:%d\n",
                base->sect_clust); /* Left sectors in cluster */
        sprintk("  *fs:0x%08X\n",
                base->fs); /* Pointer to the owner file system object */
        sprintk("  fptr:%d\n", base->fptr);             /* File R/W pointer */
        sprintk("  fsize:%d\n", base->fsize);           /* File size */
        sprintk("  org_clust:%d\n", base->org_clust);   /* File start cluster */
        sprintk("  curr_clust:%d\n", base->curr_clust); /* Current cluster */
        sprintk("  curr_sect:%d\n", base->curr_sect);   /* Current sector */
        sprintk("  dir_sect:%d\n",
                base->dir_sect); /* Sector containing the directory entry */
        sprintk(
            "  dir_ptr:0x%08X\n",
            base->dir_ptr); /* Ponter to the directory entry in the window */
    } else {
    }
}

static void testFatInodeMacro() {
    INode_t *node = (INode_t *)kmalloc(sizeof(INode_t));
    FatINode_t *finode = &node->fatinode;

    INode_t *tnode = fatinode2inode(finode, fatinode);
    sprintk("node:0x%08X finode:0x%08X tempnode:0x%08X\n", node, finode, tnode);
    kfree(node, sizeof(INode_t));
}

extern FileSystem_t *TempMainFS;
static void testFatInodeInternalPath() {
    char *out = kmalloc(128);
    char *path = "/abc/abc/123.txt";

    FatFs_t *fatfs = &TempMainFS->fatfs;
    if (fatfs != NULL) {
        char *p = construct_internal_path(fatfs, out, path);
        printk("PathTest:%s\n", p);
    }
    kfree(out, 128);
}
static void test_fat_inode() {
    char *path = "/hello.txt";

    INode_t *inode = fat_lookup_inode(&TempMainFS->fatfs, path);
    if (inode != NULL) {
        FatINode_t *fatinode = &inode->fatinode;
        printFatINode(inode);

        char *file = kmalloc(512);
        memset(file, 0, 512);
        int b = 0;
        int ret = fatbase_read(&(fatinode->fatbase_data.file), file, 512, &b);
        sprintk("read test\nret:%d \nfile:\n%s\n", ret, file);
    } else {
        sprintk("inode is null!\n");
    }
}
void testFatInode() {
    printk("FatInode Test:\n");
    sprintk("FatInode Test:\n");
    testFatInodeMacro();
    testFatInodeInternalPath();
    test_fat_inode();
}

// open
// path char -> fat_inode -> inode
// load_inode->create_inode
//