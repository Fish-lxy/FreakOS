#include "file.h"
#include "inode.h"
#include "kmalloc.h"
#include "list.h"
#include "string.h"
#include "task.h"
#include "vfs.h"

int32_t files_get_next_file_id(FileStruct_t *files);

int file_open_inc(File_t *file) {
    file->open_count++;
    return file->open_count;
}
int file_open_dec(File_t *file) {
    file->open_count--;
    return file->open_count;
}
static FileStruct_t *get_cur_files() {
    FileStruct_t *files = CurrentTask->files;
    return files;
}
static list_ptr_t *get_cur_file_list() {
    FileStruct_t *files = CurrentTask->files;
    return &files->file_list;
}
static File_t *file_alloc_init() {
    File_t *file = kmalloc(sizeof(File_t));
    if (file == NULL) {
        return NULL;
    }
    file->fd = 0;
    file->inode = NULL;
    file->open_count = 0;
    file->pos = 0;
    file->status = FD_INIT;
    file->readable = 0;
    file->writable = 0;
    initList(&file->ptr);

    return file;
}
static File_t *find_file_with_fd(int fd) {
    list_ptr_t *i;
    list_ptr_t *file_list = get_cur_file_list();
    File_t *file = NULL;
    listForEach(i, file_list) {
        file = lp2file(i, ptr);
        if (file != NULL && fd == file->fd) {
            return file;
        }
    }
    return NULL;
}
bool file_test_rw(int fd, bool readable, bool writeable) {
    int ret;
    File_t *file;
    if ((file = find_file_with_fd(fd)) == NULL) {
        return FALSE;
    }
    if (readable && !file->readable) {
        return FALSE;
    }
    if (writeable && !file->writable) {
        return FALSE;
    }
    return TRUE;
}
//-------------------------------------------------------
int file_open(char *path, uint32_t open_flag) {
    bool readable = 0;
    bool writeable = 0;
    int ret = 0;
    switch (open_flag & OPEN_MASK) {
    case OPEN_READ:
        readable = 1;
        break;
    case OPEN_WRITE:
        writeable = 1;
        break;
    case OPEN_RW:
        readable = writeable = 1;
        break;
    }

    FileStruct_t *files = get_cur_files();

    File_t *new_file = file_alloc_init();
    if (new_file == NULL) {
        ret = -FILE_MEM_ERR;
        return ret;
    }
    INode_t *inode = NULL;
    if ((ret = vfs_open(path, open_flag, &inode)) != 0) {
        ret = -FILE_OP_FAILED;
        goto failed_vfs_open;
    }

    // 查找进程中已经打开的文件
    list_ptr_t *i;
    list_ptr_t *file_list = &files->file_list;
    bool has_same = FALSE;
    lock_files(files);
    listForEach(i, file_list) {
        File_t *found_file = lp2file(i, ptr);
        sprintk("found inode:0x%08X\n", found_file->inode);
        if (found_file->inode == inode) {
            // sprintk("has Same\n");
            has_same = TRUE;
            kfree(new_file, sizeof(File_t));
            new_file = found_file;
            break;
        }
    }
    sprintk("Same:%d\n", has_same);
    unlock_files(files);

    new_file->status = FD_OPENED;
    new_file->inode = inode;
    new_file->pos = 0;
    new_file->readable = readable;
    new_file->writable = writeable;

    if (has_same != TRUE) {
        file_open_inc(new_file);
        new_file->fd = files_get_next_file_id(files);
        files_add_list(files, new_file);
    }

    return new_file->fd;

failed_vfs_open:
    kfree(new_file, sizeof(File_t));
    return ret;
}

int file_close(int fd) {
    int ret;
    File_t *file = find_file_with_fd(fd);
    if (file == NULL) {
        ret = -FILE_INVAL;
        return ret;
    }
    ret = vfs_close(file->inode);
    if (ret != VFS_OK) {
        ret = -FILE_INVAL;
    }
    file_open_dec(file);
    files_del_list(file);
    kfree(file, sizeof(File_t));
    return FILE_OK;
}

int file_read(int fd, void *buf, uint32_t blen, uint32_t *copied) {
    int ret;

    File_t *file = find_file_with_fd(fd);
    if (file == NULL) {
        ret = -FILE_INVAL;
        return ret;
    }
    if (file->readable != TRUE) {
        ret = -FILE_INVAL;
        return ret;
    }

    if (file->inode->ops->iop_read == NULL) {
        ret = -FILE_INVAL_OP;
        return ret;
    }
    uint32_t _copied = 0;
    file_open_inc(file);
    ret = file->inode->ops->iop_read(file->inode, buf, blen, &_copied);
    if (ret != 0) {
        ret = -FILE_OP_FAILED;
        goto failed;
    }

    if (file->status == FD_OPENED) {
        file->pos += blen;
    }
    *copied = _copied;

    ret = 0;
failed:
    file_open_dec(file);
    return ret;
}
int file_write(int fd, void *buf, uint32_t blen, uint32_t *copied) {
    int ret;

    File_t *file = find_file_with_fd(fd);
    if (file == NULL) {
        ret = -FILE_INVAL;
        return ret;
    }
    if (file->writable != TRUE) {
        ret = -FILE_INVAL;
        return ret;
    }

    if (file->inode->ops->iop_write == NULL) {
        ret = -FILE_INVAL_OP;
        return ret;
    }
    uint32_t _copied = 0;
    file_open_inc(file);
    ret = file->inode->ops->iop_write(file->inode, buf, blen, &_copied);
    if (ret != 0) {
        ret = -FILE_OP_FAILED;
        goto failed;
    }

    if (file->status == FD_OPENED) {
        file->pos += blen;
    }
    *copied = _copied;

    ret = 0;
failed:
    file_open_dec(file);
    return ret;
}
int file_seek(int fd, int32_t pos, uint32_t flag) {
    int ret;

    File_t *file = find_file_with_fd(fd);
    if (file == NULL) {
        ret = -FILE_INVAL;
        return ret;
    }
    file_open_inc(file);
    switch (flag) {
    case LSEEK_SET:
        break;
    case LSEEK_CUR:
        pos += file->pos;
        break;
    case LSEEK_END:
        if (file->inode->ops->iop_stat == NULL) {
            ret = -FILE_INVAL_OP;
            goto failed;
        }
        Stat_t stat;
        ret = file->inode->ops->iop_stat(file->inode, &stat);
        if (ret != 0) {
            ret = -FILE_OP_FAILED;
            goto failed;
        }
        pos += stat.size;
        break;
    default:
        ret = -FILE_INVAL;
        goto failed;
    }

    if (file->inode->ops->iop_lseek == NULL) {
        ret = -FILE_INVAL_OP;
        goto failed;
    }
    ret = file->inode->ops->iop_lseek(file->inode, pos);
    if (ret != 0) {
        ret = -FILE_OP_FAILED;
        goto failed;
    }
    file->pos = pos;
    ret = 0;

failed:
    file_open_dec(file);
    return ret;
}

int file_fsync(int fd) {
    int ret;

    File_t *file = find_file_with_fd(fd);
    if (file == NULL) {
        ret = -FILE_INVAL;
        return ret;
    }
    if (file->inode->ops->iop_sync == NULL) {
        ret = -FILE_INVAL_OP;
        return ret;
    }
    file_open_inc(file);
    ret = file->inode->ops->iop_sync(file->inode);
    if (ret != 0) {
        ret = -FILE_OP_FAILED;
        goto failed;
    }
    ret = 0;
failed:
    file_open_dec(file);
    return ret;
}

//----------------------------------------------------------
static void file_dup(File_t *to, File_t *from) {
    if (to == NULL || from == NULL) {
        panic("file dup error");
    }
    to->fd = from->fd;
    to->pos = from->pos;
    to->readable = from->readable;
    to->writable = from->writable;
    inode_ref_inc(from->inode);
    inode_open_inc(from->inode);
    to->inode = from->inode;
    to->status = FD_OPENED;
    file_open_inc(to);
}

void lock_files(FileStruct_t *files) { acquireSem(&(files->sem)); }
void unlock_files(FileStruct_t *files) { releaseSem(&(files->sem)); }

FileStruct_t *files_create() {
    FileStruct_t *files = (FileStruct_t *)kmalloc(sizeof(FileStruct_t));
    if (files == NULL) {
        return NULL;
    }
    files->pwd = NULL;
    files->ref = 0;
    files->file_next_id = 0;
    initList(&(files->file_list));
    initSem(&(files->sem), 1, "files");

    return files;
}
int32_t files_get_next_file_id(FileStruct_t *files) {
    int32_t i = files->file_next_id;
    files->file_next_id++;
    return i;
}
// TODO
void files_destroy(FileStruct_t *files) {
    if (files == NULL) {
        return;
    }
}
// TODO
void files_closeall(FileStruct_t *files) {
    if (files == NULL) {
        panic("files dup error");
    }
    list_ptr_t *i;
    list_ptr_t *file_list = &files->file_list;
    File_t *file = NULL;
    while ((i = listGetNext(file_list)) != file_list) {
        file = lp2file(i, ptr);
        if (file != NULL) {
            if (file->inode->ops->iop_sync != NULL) {
                file->inode->ops->iop_sync(file->inode);
            }
        }
        file = NULL;

        // listDel(i);
        // kfree(lp2vma(i, ptr), sizeof(VirtMemArea_t));
    }
}

void files_add_list(FileStruct_t *to, File_t *file) {
    listAdd(&(to->file_list), &(file->ptr));
}
void files_del_list(File_t *file) { listDel(&(file->ptr)); }
int files_dup(FileStruct_t *to, FileStruct_t *from) {
    if (to == NULL || from == NULL) {
        panic("files dup error");
    }
    if (to->ref != 0) {
        panic("files.ref is not 0!");
    }
    to->pwd = from->pwd;
    to->file_next_id = from->file_next_id;
    if (to->pwd != NULL) {
        inode_ref_inc(to->pwd);
    }

    list_ptr_t *i;
    list_ptr_t *file_list = &from->file_list;

    int t = 0;
    lock_files(from);
    listForEach(i, file_list) {
        t++;
        sprintk("cp file\n");
        File_t *from_file = lp2file(i, ptr);
        File_t *to_file = file_alloc_init();
        file_dup(to_file, from_file);
        files_add_list(to, to_file);
    }
    unlock_files(from);
    // sprintk("t=%d\n", t);
    return 0;
}

int files_get_ref(FileStruct_t *files) { return files->ref; }
int files_ref_inc(FileStruct_t *files) {
    files->ref++;
    return files->ref;
}
int files_ref_dec(FileStruct_t *files) {
    files->ref--;
    return files->ref;
}
//----------------------------------------------------------------
void sprintCurFiles() {
    sprintk("Files in Task %d\n", CurrentTask->tid);
    FileStruct_t *cur_files = get_cur_files();
    if (cur_files == NULL) {
        sprintk("  files: cur files is null!\n");
        return;
    }
    list_ptr_t *file_list = &cur_files->file_list;
    list_ptr_t *i;
    listForEach(i, file_list) {
        File_t *file = lp2file(i, ptr);
        sprintk("file in 0x%08X", file);
        sprintk("  fd:%d\n", file->fd);
        sprintk("  status:%d\n", file->status);
        sprintk("  readable:%d\n", file->readable);
        sprintk("  writable:%d\n", file->writable);
        sprintk("  inode:0x%08X\n", file->inode);
    }
}
void test_file_open() {
    sprintk("TEST FILE OPEN:\n");
    int ret = file_open("/stdin", OPEN_READ);
    ret = file_open("/stdout", OPEN_WRITE);
    //ret = file_open("/fs0/hello", OPEN_WRITE | OPEN_READ | OPEN_CREATE);
    sprintk("open ret:%d\n\n", ret);
    // ret = file_open("/fs0/newfile",OPEN_RW);
    sprintCurFiles();
}
void test_stdfile_rw() {
    sprintk("TEST FILE RW:\n");
    printk("TEST FILE RW:\n");
    char *str = "Hello!\n";
    int len = strlen(str);
    int copied = 0;
    int ret = file_write(1, str, len, &copied); // stdout
    sprintk("rw ret:%d\n", ret);

    // char c;
    // for (;;) {
    //     ret = file_read(0, &c, 1, &copied); // stdin
    //     sprintk("%c", c);
    // }
}
void test_regularfile_rw() {
    sprintk("TEST regular FILE RW:\n");
    printk("TEST regularFILE RW:\n");
    char *str = "Hello!\n";
    int len = strlen(str);
    int copied = 0;
    int ret;
    int fd = file_open("/fs0/hello.txt", OPEN_RW);
    ret = file_write(fd, str, len, &copied);
    sprintk(" w ret:%d\n", ret);
    ret = file_seek(fd, 0,LSEEK_SET);
    sprintk(" w ret:%d\n", ret);

    sprintk(" file content:\n");
    char c;
    for (int i = 0; i < 5; i++) {
        ret = file_read(fd, &c, 1, &copied);
        //sprintk(" r ret:%d\n", ret);
        sprintk("%c", c);
    }
    file_fsync(fd);
}
void test_file() {
    sprintk("\n\nTEST FILE:\n");
    test_file_open();
    test_stdfile_rw();

    test_regularfile_rw();
}
