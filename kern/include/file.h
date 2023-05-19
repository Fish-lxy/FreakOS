#ifndef __FILE_H_
#define __FILE_H_

#include "list.h"
#include "sem.h"
#include "types.h"

struct INode_t;
typedef struct INode_t INode_t;
struct Stat_t;
typedef struct Stat_t Stat_t;

#define FS_MAX_FPATH_LEN 4095

#define NO_FD 65536

#define OPEN_MASK 0xFF
#define OPEN_READ 0x0
#define OPEN_WRITE 0x1
#define OPEN_RW 0x2
#define OPEN_CREATE 0x4
#define OPEN_EXIST 0x8
#define OPEN_TRUNC 0x10

#define LSEEK_SET 0 // seek relative to beginning of file
#define LSEEK_CUR 1 // seek relative to current position in file
#define LSEEK_END 2 // seek relative to end of file

#define FILE_OK 0
#define FILE_INVAL 1
#define FILE_INVAL_OP 2
#define FILE_NOT_EXIST 3
#define FILE_EXIST 4
#define FILE_OP_FAILED 5
#define FILE_PNULL 6
#define FILE_MEM_ERR 7;

typedef struct File_t {
    enum {
        // 文件状态: 不存在/已初始化/已打开/已关闭
        FD_NONE,
        FD_INIT,
        FD_OPENED,
        FD_CLOSED,
    } status;
    bool readable;
    bool writable;
    int fd;         // 文件在 filemap 中的索引值
    int32_t pos;    // 当前位置
    INode_t *inode; // 该文件对应的 inode 指针
    int open_count; // 打开此文件的次数

    list_ptr_t ptr;
} File_t;

#define lp2file(lp, member) to_struct((lp), File_t, member)

typedef struct FileStruct_t {
    INode_t *pwd;
    list_ptr_t file_list;
    int32_t ref;
    int32_t file_next_id;
    Semaphore_t sem;

} FileStruct_t;

// file
int file_open_inc(File_t *file);
int file_open_dec(File_t *file);

bool file_test_rw(int fd, bool readable, bool writeable);
int file_open(char *path, uint32_t open_flag);
int file_close(int fd);
int file_read(int fd, void *buf, uint32_t blen, uint32_t *copied);
int file_write(int fd, void *buf, uint32_t blen, uint32_t *copied);
int file_seek(int fd, int32_t pos, uint32_t flag);
int file_fstat(int fd, Stat_t *stat);
int file_fsync(int fd);

// files
void lock_files(FileStruct_t *files);
void unlock_files(FileStruct_t *files);

FileStruct_t *files_create();
void files_destroy(FileStruct_t *files);
void files_closeall(FileStruct_t *files);
int files_dup(FileStruct_t *to, FileStruct_t *from);

int files_get_ref(FileStruct_t *files);
int files_ref_inc(FileStruct_t *files);
int files_ref_dec(FileStruct_t *files);

void test_file();
void sprintCurFiles();

#endif