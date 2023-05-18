#include "bcache.h"
#include "block.h"
#include "debug.h"
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

static const INodeOps_t fat_file_op;
static const INodeOps_t fat_dir_op;

static INodeOps_t *get_fat_inode_ops(FATINodeType_e type);
static uint32_t getNewFatInodeId(FatFs_t *fatfs);
static void fat_add_inode_list(FatFs_t *fatfs, FatINode_t *fat_inode);
static void fat_del_inode_list(FatFs_t *fatfs, FatINode_t *fat_inode);
static char *bulid_internal_path(FatFs_t *fatfs, char *dest, char *path);
static INode_t *find_list_fat_inode_withID_nolock(FatFs_t *fatfs, uint32_t id);
static INode_t *
find_list_fatinode_with_internal_path_nolock(FatFs_t *fatfs,
                                             const char *inpath);
static INode_t *fat_create_inode(FatFs_t *fatfs);
static INode_t *construct_fatinode_info(INode_t *inode, FatFs_t *fatfs,
                                        FATINodeType_e type, uint32_t flag,
                                        char *inpath);
INode_t *fat_find_inode(FatFs_t *fatfs, char *path);

static INode_t *fat_create_inode2(FatFs_t *fatfs,
                                  FATBase_InodeData_u *base_data,
                                  FATINodeType_e type);
static INode_t *fat_load_inode(FatFs_t *fatfs, uint32_t id);

extern FileSystem_t *TempMainFS;
void printFatfsAllINode(FatFs_t *fatfs);

//---------------------------------------------------------------------
static void ________________space01________________();
//---------------------------------------------------------------------

static void fat_add_inode_list(FatFs_t *fatfs, FatINode_t *fat_inode) {
    // sprintk("fataddlist :lp:0x%08X",&(fat_inode->list_ptr));
    listAdd(&(fatfs->inode_list), &(fat_inode->list_ptr));
}
static void fat_del_inode_list(FatFs_t *fatfs, FatINode_t *fat_inode) {
    listDel(&(fat_inode->list_ptr));
}
static int free_fat_inode(INode_t *inode) {
    if (inode != NULL) {
        kfree(inode->fatinode.internal_path, INTERNAL_PATH_LEN);
        return 0;
    }
    return 1;
}
static bool check_prefix(char *str) {
    if (str == NULL) {
        return FALSE;
    }
    if ((str[0] >= '0' && str[0] <= '9') && str[1] == ':') {
        return TRUE;
    }
    return FALSE;
}

// 构造fat驱动所需内部路径
// example: abc/abc -> 0:/abc/abc
static char *bulid_internal_path(FatFs_t *fatfs, char *dest, char *path) {
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

// 兼容层文件read/write
static int fat_file_rw_nolock(FatINode_t *fat_inode, void *buf, uint32_t len,
                              uint32_t *copied, bool write) {
    int ret = 0;

    FATBase_FILE *base_fp = &fat_inode->fatbase_data.file;
    if (write != TRUE) {
        ret = fatbase_read(base_fp, buf, len, copied);
    } else {
        ret = fatbase_write(base_fp, buf, len, copied);
    }
    return ret;
}
static int fat_file_rw(INode_t *inode, void *buf, uint32_t len,
                       uint32_t *copied, bool write) {
    int ret = 0;
    if (inode == NULL || buf == NULL) {
        ret = -INODE_PNULL;
        return ret;
    }
    if (inode->fatinode.fatinode_type != FAT_Type_File) {
        ret = -INODE_NOT_FILE;
        return ret;
    }

    FatINode_t *fat_inode = &inode->fatinode;
    lockFatInode(fat_inode);
    { ret = fat_file_rw_nolock(fat_inode, buf, len, copied, write); }
    unlockFatInode(fat_inode);
    return ret;
}
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
    if (fatfs == NULL || inpath == NULL) {
        return NULL;
    }
    list_ptr_t *list = &fatfs->inode_list;
    list_ptr_t *i = NULL;
    FatINode_t *tempi = NULL;
    INode_t *inode = NULL;
    listForEach(i, list) {
        tempi = lp2fatinode(i, list_ptr);
        // sprintk("inode in list:0x%08X\n", tempi);
        // sprintk(
        //     "inode.inpath:%s\n",
        //     (tempi->internal_path == NULL ? "NULL!" : tempi->internal_path));
        // sprintk("inpath:%s\n", inpath);
        if (strcmp(tempi->internal_path, inpath) == 0) {
            inode = fatinode2inode(tempi, fatinode);
            return inode;
        }
    }
    return NULL;
}
// 建立新的inode
// static INode_t *fat_create_inode(FatFs_t *fatfs) {
//     INode_t *inode = NULL; // in heap
//     if ((inode = inode_alloc(FAT_Type)) != NULL) {
//         inode_init(inode, get_fat_inode_ops(), fatfs2fs(fatfs, fatfs));
//         return inode;
//     }
//     return NULL;
// }

// 构造FatINode内部数据
// Warning: inpath 指向的字符串必须在堆上分配，不允许传入栈上分配的临时变量
static INode_t *construct_fatinode_info(INode_t *inode, FatFs_t *fatfs,
                                        FATINodeType_e type, uint32_t flag,
                                        char *inpath) {
    if (inode != NULL) {
        FatINode_t *fatinode = &inode->fatinode;
        fatinode->internal_path = inpath;
        fatinode->fatfs = fatfs;
        fatinode->fatinode_type = type;
        fatinode->flag = flag;
        fatinode->id = getNewFatInodeId(fatfs);
        initSem(&fatinode->sem, 1, "fatinode");
        return inode;
    }
    return NULL;
}

//  从内存和磁盘查找文件INode，
//  内存存在则返回INode，内存不存在但磁盘存在则创建后返回，磁盘中不存在则返回NULL
//  不会在磁盘上建立新文件
//  path 不可是内部路径
//  path 指向的内存允许为栈上分配或堆上分配
INode_t *fat_find_inode(FatFs_t *fatfs, char *path) {
    // sprintk("find1 lock 0x%08X\n", fatfs);
    lockFatFS(fatfs);

    if (fatfs == NULL || path == NULL) {
        goto failed_unlock;
    }
    // sprintk("find1ok\n");

    // 在堆上分配新的路径字符串, 构造内部路径
    char *internal_path; // in heap
    if ((internal_path = (char *)kmalloc(INTERNAL_PATH_LEN)) == NULL) {
        goto failed_unlock;
    }
    memset(internal_path, 0, INTERNAL_PATH_LEN);
    bulid_internal_path(fatfs, internal_path, path);

    // sprintk("find2ok %s\n", internal_path);

    // 在fatfs的inode链表中查找
    INode_t *inode = NULL;
    if ((inode = find_list_fatinode_with_internal_path_nolock(
             fatfs, internal_path)) != NULL) {
        // sprintk("find3found inode in list:0x%08X\n", inode);
        goto ok_free_inpath;
    }
    // sprintk("find3ok find inode in list:0x%08X\n", inode);

    // 内存中没有需要的inode
    // 读磁盘 构造inode
    // 申请空inode
    if ((inode = inode_alloc(FAT_Type)) == NULL) {
        goto failed_inpath;
    }
    FileSystem_t *fs = fatfs2fs(fatfs, fatfs);
    inode_init(inode, NULL, fs);
    // sprintk("find4ok alloc inode:0x%08X fs:0x%08X\n", inode, fs);
    inode->fs = fs;
    // sprintk(" inode.fs:0x%08X\n",inode->fs);

    // 利用fat底层驱动的f_open，读取FATBase_InodeData
    FATBase_InodeData_u *base_node_data = &(inode->fatinode.fatbase_data);
    FATINodeType_e type;
    int open_ret = 0;
    int open_type = 0;
    if ((open_ret = fatbase_open(&(base_node_data->file), internal_path,
                                 FA_READ | FA_WRITE)) == FR_OK) {
        type = FAT_Type_File;
    } else if ((open_ret = fatbase_opendir(&(base_node_data->dir),
                                           internal_path)) == FR_OK) {
        type = FAT_Type_Dir;
    } else {
        // sprintk("find5 err open ret:%d \n", open_ret);
        goto failed_f_open;
    }
    inode->ops = get_fat_inode_ops(type);

    construct_fatinode_info(inode, fatfs, type, 0, internal_path);

    fat_add_inode_list(fatfs, &(inode->fatinode));
    // sprintk("find5ok type:%d \n", type);

out:
    unlockFatFS(fatfs);
    return inode;
ok_free_inpath:
    unlockFatFS(fatfs);
    kfree(internal_path, INTERNAL_PATH_LEN);
    return inode;

failed_f_open:
    inode_free(inode);
    inode = NULL;
failed_inpath:
    kfree(internal_path, INTERNAL_PATH_LEN);
failed_unlock:
    unlockFatFS(fatfs);
    return NULL;
}

int32_t fat_get_fsize(INode_t *inode) {
    if (inode == NULL) {
        return -1;
    }
    if (inode->fatinode.fatinode_type != FAT_Type_File) {
        return -2;
    }
    return inode->fatinode.fatbase_data.file.fsize;
}
//---------------------------------------------------------------------
static void ________________space02________________();
//---------------------------------------------------------------------

// 查找基于传入inode的相对路径的inode
int fat_lookup(INode_t *inode, char *relative_path, INode_t **inode_out) {
    if (inode == NULL || relative_path == NULL) {
        return -INODE_PNULL;
    }
    if (inode->fatinode.fatinode_type != FAT_Type_Dir) {
        return -INDOE_NOT_DIR;
    }

    int ret = 0;
    FatFs_t *fatfs = &inode->fs->fatfs;

    char *inode_in_path = inode->fatinode.internal_path;

    // fat_find_inode要求的路径参数不能有前缀，所以跳过前缀
    inode_in_path = inode_in_path + 2; //
    // sprintk("lookip01:inode_in_path:%s\n", inode_in_path);

    char *new_in_path; // in heap
    if ((new_in_path = (char *)kmalloc(INTERNAL_PATH_LEN)) == NULL) {
        ret = -INODE_INTERNAL_ERR;
        goto failed;
    }
    memset(new_in_path, 0, INTERNAL_PATH_LEN);

    // 拼接路径
    // 先复制inode的路径
    strcpy(new_in_path, inode_in_path);
    int32_t new_in_path_len = strlen(new_in_path);
    // 在拼接处补'/'
    if (new_in_path[new_in_path_len - 1] != '/' && relative_path[0] != '/') {
        strcat(new_in_path, "/");
    }
    strcat(new_in_path, relative_path);

    // sprintk("lookup02 new_path:%s\n", new_in_path);

    INode_t *out = NULL;
    if ((out = fat_find_inode(fatfs, new_in_path)) == NULL) {
        ret = -INODE_NOT_EXIST;
        goto failed_lookup_inode;
    }
    // sprintk("lookup03 ok:\n");
    *inode_out = out;

    sprintk("%s\n", new_in_path);
out:
    kfree(new_in_path, INTERNAL_PATH_LEN);
    return 0;

failed_lookup_inode:
    kfree(new_in_path, INTERNAL_PATH_LEN);
failed:
    return ret;
}

// 打开/创建文件
int fat_openfile(INode_t *inode, int flag) { return 0; }
int fat_write(INode_t *inode, void *buf, uint32_t len, uint32_t *copied) {
    int ret = 0;
    ret = fat_file_rw(inode, buf, len, copied, TRUE);
    return ret;
}
// 作为函数指针存入INode
int fat_read(INode_t *inode, void *buf, uint32_t len, uint32_t *copied) {
    int ret = 0;
    ret = fat_file_rw(inode, buf, len, copied, FALSE);
    return ret;
}

int fat_lseek(INode_t *inode, int32_t off) {
    if (inode == NULL) {
        return -INODE_PNULL;
    }
    if (inode->fatinode.fatinode_type != FAT_Type_File) {
        return -INODE_NOT_FILE;
    }
    FatINode_t *fatinode = &inode->fatinode;
    FATBase_FILE *base_fp = &fatinode->fatbase_data.file;
    int ret = 0;
    lockFatInode(fatinode);
    ret = fatbase_lseek(base_fp, off);
    unlockFatInode(fatinode);
    return ret;
}
// 将改变文件大小为len
int fat_truncate(INode_t *inode, uint32_t len) {
    if (inode == NULL) {
        return -INODE_PNULL;
    }
    if (inode->fatinode.fatinode_type != FAT_Type_File) {
        return -INODE_NOT_FILE;
    }
    int ret = 0;
    FatINode_t *fatinode = &inode->fatinode;
    FATBase_FILE *base_fp = &fatinode->fatbase_data.file;
    char *inpath = fatinode->internal_path;

    lockFatInode(fatinode);
    {
        int fsize = base_fp->fsize;
        sprintk("truncate :file size:%d\n", fsize);
        int save_fptr = 0;
        fatbase_tell(base_fp, &save_fptr);

        if (len > fsize) { // len 大于文件长度,扩展文件大小
            fatbase_lseek(base_fp, len);
        } else { // len 小于文件长度,收缩文件大小

            fatbase_lseek(base_fp, len);
            fatbase_truncate(base_fp);
        }

        fatbase_lseek(base_fp, save_fptr);
    }
    unlockFatInode(fatinode);
    return ret;
}
int fat_create(INode_t *inode, char *name, uint32_t mode) {
    if (inode == NULL || name == NULL) {
        return -INODE_PNULL;
    }
    if (inode->fatinode.fatinode_type != FAT_Type_Dir) {
        return -INDOE_NOT_DIR;
    }
    int ret = 0;

    // 是否覆盖已有文件
    uint32_t is_exist = ((mode & INODE_CREATE_MASK) == INODE_CREATE_EXIST);
    sprintk("create01:exist::%d\n", is_exist);

    char *inode_in_path = inode->fatinode.internal_path;
    sprintk("create02:inode_in_path:%s\n", inode_in_path);

    char *new_in_path; // in heap
    FATBase_FILE *new_file;
    if ((new_in_path = (char *)kmalloc(INTERNAL_PATH_LEN)) == NULL) {
        ret = -INODE_INTERNAL_ERR;
        return ret;
    }
    if ((new_file = (FATBase_FILE *)kmalloc(sizeof(FATBase_FILE))) == NULL) {
        ret = -INODE_INTERNAL_ERR;
        goto failed_file_kalloc;
    }
    memset(new_in_path, 0, INTERNAL_PATH_LEN);

    // 先复制inode的路径
    strcpy(new_in_path, inode_in_path);
    int32_t new_in_path_len = strlen(new_in_path);
    // 在拼接处补'/'
    if (new_in_path[new_in_path_len - 1] != '/' && name[0] != '/') {
        strcat(new_in_path, "/");
    }
    // 拼接路径
    strcat(new_in_path, name);
    sprintk("create03:new_path:%s\n", new_in_path);

    if (is_exist == TRUE) { // 覆盖已有文件
        if ((ret = fatbase_open(new_file, new_in_path, FA_CREATE_ALWAYS)) !=
            FR_OK) {
            ret = INODE_OK;
            goto failed_create;
        }
    } else {
        ret = fatbase_open(new_file, new_in_path, FA_CREATE_NEW);
        if (ret == FR_OK) {
            ret = INODE_OK;
            goto ok;
        } else if (ret == FR_EXIST) {
            ret = -INODE_EXIST;
            goto ok_existed;
        } else {
            ret == -INODE_INTERNAL_ERR;
            goto failed_create;
        }
    }

ok:
    fatbase_close(new_file);
ok_existed:
failed_create:
    kfree(new_file, sizeof(FATBase_FILE));
failed_file_kalloc:
    kfree(new_in_path, INTERNAL_PATH_LEN);
out:
    sprintk("create03:out ret:%d\n", ret);
    return ret;
}

int fat_opendir(INode_t *inode, int flag) { return 0; }
int fat_readdir(INode_t *inode, INode_t **inode_out) {
    if (inode == NULL) {
        return -INODE_PNULL;
    }
    if (inode->fatinode.fatinode_type != FAT_Type_Dir) {
        return -INDOE_NOT_DIR;
    }
    int ret = 0;
    FatINode_t *fatinode = &inode->fatinode;
    FATBase_DIR *base_fp = &fatinode->fatbase_data.dir;
    // rewind dir
    if (inode_out == NULL) {
        // sprintk("readdir rewind 01\n");
        const char *inpath = fatinode->internal_path;
        if ((ret = fatbase_opendir(base_fp, inpath)) != FR_OK) {
            ret = -INODE_INTERNAL_ERR;
            return ret;
        }
        return ret;
    }

    FATBase_FILINFO fatinfo;
    if (ret = fatbase_readdir(base_fp, &fatinfo) != FR_OK) {
        ret = -INODE_INTERNAL_ERR;
        return ret;
    }
    char *info_filename = fatinfo.fname;
    if (info_filename[0] == 0) {
        sprintk("End of Dir!");
        *inode_out = NULL;
        return INODE_OK;
    }

    if (ret = fat_lookup(inode, info_filename, inode_out) != INODE_OK) {
        return ret;
    }
    return ret;
}
int fat_mkdir(INode_t *inode, char *dir_name) { // 创建目录，在inode下创建新目录
    if (inode == NULL || dir_name == NULL) {
        return -INODE_PNULL;
    }
    if (inode->fatinode.fatinode_type != FAT_Type_Dir) {
        return -INDOE_NOT_DIR;
    }
    int ret = 0;
    FatINode_t *fatinode = &inode->fatinode;
    FATBase_DIR *base_fp = &fatinode->fatbase_data.dir;

    char *inode_in_path = inode->fatinode.internal_path;
    sprintk("mkdir02:inode_in_path:%s\n", inode_in_path);

    char *new_path; // in heap
    FATBase_FILE *new_file;
    if ((new_path = (char *)kmalloc(INTERNAL_PATH_LEN)) == NULL) {
        ret = -INODE_INTERNAL_ERR;
        return ret;
    }
    memset(new_path, 0, INTERNAL_PATH_LEN);

    // 先复制inode的路径
    strcpy(new_path, inode_in_path);
    int32_t new_in_path_len = strlen(new_path);
    // 在拼接处补'/'
    if (new_path[new_in_path_len - 1] != '/' && dir_name[0] != '/') {
        strcat(new_path, "/");
    }
    // 拼接路径
    strcat(new_path, dir_name);
    sprintk("mkdir03:new_path:%s\n", new_path);

    ret = fatbase_mkdir(new_path);
    if (ret == FR_OK) {
        ret = INODE_OK;
    } else if (ret == FR_EXIST) {
        ret = -INODE_EXIST;
    } else {
        ret = -INODE_INTERNAL_ERR;
    }

    kfree(new_path, INTERNAL_PATH_LEN);
    return ret;
}

// 同步此INode的所有数据到磁盘，然后从内存中移除INode
int fat_release(INode_t *inode) {
    if (inode == NULL) {
        return -INODE_PNULL;
    }
    int ret;

    FatINode_t *fatinode = &inode->fatinode;
    FatFs_t *fatfs = fatinode->fatfs;

    if (fatinode->fatinode_type == FAT_Type_File) {
        FATBase_FILE *base_fp = &fatinode->fatbase_data.file;
        fatbase_close(base_fp);
    } else if (fatinode->fatinode_type == FAT_Type_Dir) {
        FATBase_DIR *base_fp = &fatinode->fatbase_data.dir;
        // do nothing;
    } else {
        ret = -INODE_INTERNAL_ERR;
        return ret;
    }

    kfree(fatinode->internal_path, INTERNAL_PATH_LEN);
    fat_del_inode_list(fatfs, fatinode);
    inode_free(inode);

    ret = INODE_OK;
    return ret;
}
// 从磁盘上删除INode下的文件或目录
int fat_delete(INode_t *inode, char *name) {
    return 0;
    // TODO
}
int fat_rename(INode_t *inode, char *new) {
    return 0;
    // TODO
}
int fat_stat(INode_t *inode, Stat_t *stat) {
    if (inode == NULL || stat == NULL) {
        return -INODE_PNULL;
    }
    int ret = 0;
    FATBase_FILINFO file_info;
    const char *file_path = inode->fatinode.internal_path;
    if ((ret = fatbase_stat(file_path, &file_info)) != FR_OK) {
        ret = -INODE_INTERNAL_ERR;
        return ret;
    }
    stat->size = file_info.fsize;

    uint32_t is_dir = ((file_info.fattrib & AM_MASK) == AM_DIR);
    if (is_dir == 1) {
        stat->attr |= INODE_TYPE_DIR;
    } else {
        stat->attr |= INODE_TYPE_REGULAR;
    }
    ret = INODE_OK;
    return ret;
}
int fat_sync(INode_t *inode) {
    if (inode == NULL) {
        return -INODE_PNULL;
    }
    if (inode->fatinode.fatinode_type != FAT_Type_File) {
        return -INODE_NOT_FILE;
    }
    FatINode_t *fatinode = &inode->fatinode;
    FATBase_FILE *base_fp = &fatinode->fatbase_data.file;
    int ret = 0;

    lockFatInode(fatinode);
    {
        sprintk("fat_sync:同步缓冲区\n");
        ret = fatbase_sync(base_fp);
        sync_all_bcache();
        sprintk("fat_sync:同步结束\n");
    }
    unlockFatInode(fatinode);
    return ret;
}
int fat_get_type(INode_t *inode, int32_t *type_out) {
    if (inode == NULL || type_out == NULL) {
        return -INODE_PNULL;
    }
    FATINodeType_e base_type = inode->fatinode.fatinode_type;
    switch (base_type) {
    case FAT_Type_File:
        *type_out = INODE_TYPE_REGULAR;
        return INODE_OK;
    case FAT_Type_Dir:
        *type_out = INODE_TYPE_DIR;
        return INODE_OK;
    default:
        return -INODE_INTERNAL_ERR;
    }
}

static INodeOps_t *get_fat_inode_ops(FATINodeType_e type) {
    switch (type) {
    case FAT_Type_File:
        return &fat_file_op;
    case FAT_Type_Dir:
        return &fat_dir_op;
    default:
        panic("type is Wrong");
    }
}

static const INodeOps_t fat_file_op = {
    .inode_magic = INODE_OPS_MAGIC,
    .iop_lookup = fat_lookup,
    .iop_open = fat_openfile,
    .iop_read = fat_read,
    .iop_write = fat_write,
    .iop_lseek = fat_lseek,
    .iop_truncate = fat_truncate,
    .iop_create = NULL,
    .iop_readdir = NULL,
    .iop_mkdir = NULL,
    .iop_dev_io = NULL,
    .iop_dev_ioctl = NULL,
    .iop_release = fat_release,
    .iop_delete = fat_delete,
    .iop_rename = fat_rename,
    .iop_stat = fat_stat,
    .iop_sync = fat_sync,
    .iop_get_type = fat_get_type,
};
static const INodeOps_t fat_dir_op = {
    .inode_magic = INODE_OPS_MAGIC,
    .iop_lookup = fat_lookup,
    .iop_open = fat_opendir,
    .iop_read = NULL,
    .iop_write = NULL,
    .iop_lseek = NULL,
    .iop_truncate = NULL,
    .iop_create = fat_create,
    .iop_readdir = fat_readdir,
    .iop_mkdir = fat_mkdir,
    .iop_dev_io = NULL,
    .iop_dev_ioctl = NULL,
    .iop_release = fat_release,
    .iop_delete = fat_delete,
    .iop_rename = fat_rename,
    .iop_stat = fat_stat,
    .iop_sync = NULL,
    .iop_get_type = fat_get_type,
};

//------------------------------------------------------------------------------------------------
static void ________________TEST02________________();
// TEST
//------------------------------------------------------------------------------------------------
void printFatINode(INode_t *inode) {

    sprintk("print inode in 0x%08X:\n", inode);
    if (inode == NULL) {
        sprintk(" inode is null!\n");
        return;
    }
    if (inode->inode_type != FAT_Type) {
        sprintk("inode is not FAT_Type! type: %d:\n", inode->inode_type);
        return;
    }
    FatINode_t *fatinode = &inode->fatinode;
    sprintk(" i_fs:0x%08X", inode->fs);
    sprintk(" id:%d\n", fatinode->id);
    sprintk(" type:%s/%d\n",
            fatinode->fatinode_type == FAT_Type_File ? "file" : "dir",
            fatinode->fatinode_type);

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
        FATBase_DIR *base = &fatinode->fatbase_data;
        sprintk("  id:%d\n", base->id);       /* Owner file system mount ID */
        sprintk("  index:%d\n", base->index); /* Current index */
        sprintk("  *fs:0x%08X\n",
                base->fs); /* Pointer to the owner file system object */
        sprintk("  sclust:%d\n", base->sclust); /* Start cluster */
        sprintk("  clust:%d\n", base->clust);   /* Current cluster */
        sprintk("  sect:%d\n", base->sect);     /* Current sector */
    }
    sprintk(" inpath in 0x%08X", fatinode->internal_path);
    sprintk(" inpath:%s\n", fatinode->internal_path);
}
void printFatfsAllINode(FatFs_t *fatfs) {
    sprintk("PrintAllINode:\n");
    list_ptr_t *list = &fatfs->inode_list;
    list_ptr_t *i = NULL;
    FatINode_t *fati = NULL;
    INode_t *inode = NULL;
    listForEach(i, list) {
        fati = lp2fatinode(i, list_ptr);
        sprintk("print fati in 0x%08X:\n", fati);
        sprintk("print fati.listptr in 0x%08X:\n", &fati->list_ptr);

        inode = fatinode2inode(fati, fatinode);
        sprintk("print inode.fatinode in 0x%08X:\n", &inode->fatinode);
        printFatINode(inode);
    }
}
void read_sprintFatInodeContent(INode_t *inode) {
    if (inode == NULL) {
        sprintk(" inode is null!\n");
        return;
    }
    FatINode_t *fatinode = &inode->fatinode;
    if (fatinode->fatinode_type == FAT_Type_File) {
        int32_t fsize = fat_get_fsize(inode);
        char *buf2 = kmalloc(fsize + 1);
        memset(buf2, 0, fsize + 1);
        int copied=0;
        int ret = fat_read(inode, buf2, 512,&copied);
        fat_lseek(inode, 0);

        sprintk("read ret:%d fsize:%d\n", ret, fsize);
        sprintk("file content:\n");
        for (int i = 0; i < fsize; i++) {
            if (buf2[i] == 0) {
                sprintk("\\0");
            } else
                sprintk("%c", buf2[i]);
        }
        sprintk("(EOF)\n");
        kfree(buf2, fsize + 1);
    }
}

static void testFatInodeMacro() {
    INode_t *node = (INode_t *)kmalloc(sizeof(INode_t));
    FatINode_t *finode = &node->fatinode;

    INode_t *tnode = fatinode2inode(finode, fatinode);
    sprintk("node:0x%08X finode:0x%08X tempnode:0x%08X\n", node, finode, tnode);
    kfree(node, sizeof(INode_t));
}

static void testFatInodeInternalPath() {
    char *out = kmalloc(128);
    char *path = "/abc/abc/123.txt";

    FatFs_t *fatfs = &TempMainFS->fatfs;
    if (fatfs != NULL) {
        char *p = bulid_internal_path(fatfs, out, path);
        printk("PathTest:%s\n", p);
    }
    kfree(out, 128);
}
static void test_fat_inode() {
    char *inode1_path = "/";

    INode_t *inode = fat_find_inode(&TempMainFS->fatfs, inode1_path);
    if (inode != NULL) {
        FatINode_t *fatinode = &inode->fatinode;
        printFatINode(inode);
        // char *file = kmalloc(512);
        // memset(file, 0, 512);
        // int b = 0;
        // int ret = fatbase_read(&(fatinode->fatbase_data.file), file, 512,
        // &b); sprintk("read test\nret:%d \nfile:\n%s\n", ret, file);
    } else {
        sprintk("inode is null!\n");
    }

    // printFatfsAllINode(&TempMainFS->fatfs);
    sprintk("\ninode2:\n");

    INode_t *inode2 = NULL;
    fat_lookup(inode, "main.c", &inode2);

    if (inode2 != NULL) {
        FatINode_t *fatinode = &inode2->fatinode;
        printFatINode(inode2);
    } else {
        sprintk("inode2 is null!\n");
    }
}
void testRootInode() {
    sprintk("Root INode Test:\n");
    INode_t *inode = get_fatfs_root_inode(TempMainFS);
    if (inode != NULL) {
        FatINode_t *fatinode = &inode->fatinode;
        printFatINode(inode);

    } else {
        sprintk("root inode is null!\n");
    }

    INode_t *inode2 = NULL;
    fat_lookup(inode, "dir01/file01.txt", &inode2);

    if (inode2 != NULL) {
        FatINode_t *fatinode = &inode2->fatinode;
        printFatINode(inode2);
    } else {
        sprintk("inode2 is null!\n");
    }
}
void testInodeRW() {
    sprintk("Root INode:\n");
    INode_t *inode = get_fatfs_root_inode(TempMainFS);
    if (inode != NULL) {
        FatINode_t *fatinode = &inode->fatinode;
        printFatINode(inode);

    } else {
        sprintk("root inode is null!\n");
    }

    INode_t *inode_rw = NULL;
    fat_lookup(inode, "dir01/file01.txt", &inode_rw);

    if (inode_rw != NULL) {
        FatINode_t *fatinode = &inode_rw->fatinode;
        printFatINode(inode_rw);
    } else {
        sprintk("inode2 is null!\n");
    }

    int ret = 0;

    // 截断文件为0
    fat_truncate(inode_rw, 0);

    read_sprintFatInodeContent(inode_rw);

    char *buf = kmalloc(512);
    memset(buf, 0, 512);
    strcpy(buf, "I'm file01");
    int buf_len = strlen(buf);

    int copied = 0;
    ret = fat_write(inode_rw, buf, buf_len,&copied);
    fat_lseek(inode_rw, 0);
    sprintk("write ret:%d\nbuf:%s\n", ret, buf);

    fat_truncate(inode_rw, 128);
    fat_sync(inode_rw);

    read_sprintFatInodeContent(inode_rw);
}
void testCreate() {
    sprintk("Root INode:\n");
    INode_t *root = get_fatfs_root_inode(TempMainFS);
    printFatINode(root);
    if (root == NULL) {
        sprintk("root inode is null!\n");
    }

    int ret = fat_create(root, "newfile", INODE_CREATE_NEW);
    if (ret == 5) {
        sprintk("create faile:file existed\n");
    }
    sprintk("create ret:%d\n", ret);

    sync_all_bcache();
}
void testReaddir() {
    sprintk("Dir Read Test:\n");
    sprintk("Root INode:\n");
    int ret = 0;
    INode_t *root = get_fatfs_root_inode(TempMainFS);
    printFatINode(root);

    INode_t *inode = NULL;
    ret = fat_lookup(root, "dir01", &inode);
    sprintk("ret:%d\n", ret);
    printFatINode(inode);

    INode_t *inode_out = NULL;
    ret = fat_readdir(inode, &inode_out);
    sprintk("ret:%d\n", ret);
    printFatINode(inode_out);

    ret = fat_readdir(inode, &inode_out);
    sprintk("ret:%d\n", ret);
    printFatINode(inode_out);

    ret = fat_readdir(inode, NULL);
    sprintk("rewind dir ret:%d\n", ret);

    ret = fat_readdir(inode, &inode_out);
    sprintk("ret:%d\n", ret);
    printFatINode(inode_out);
}
void testMkdir() {
    sprintk("Dir Read Test:\n");
    sprintk("Root INode:\n");
    int ret = 0;
    INode_t *root = get_fatfs_root_inode(TempMainFS);
    printFatINode(root);

    ret = fat_mkdir(root, "dirtest");
    sprintk("ret:%d\n", ret);

    INode_t *inode = NULL;
    ret = fat_lookup(root, "dirtest", &inode);
    sprintk("ret:%d\n", ret);
    printFatINode(inode);

    sync_all_bcache();
}
void testFatInode() {

    printk("\nFatInode Test:\n");
    sprintk("\nFatInode Test:\n");
    // testFatInodeMacro();
    // testFatInodeInternalPath();
    // test_fat_inode();
    // testRootInode();
     testInodeRW();
    // testCreate();
    testMkdir();
    sprintk("\nFatInode Test OK\n");
}

// open
// path char -> fat_inode -> inode
// load_inode->create_inode
//