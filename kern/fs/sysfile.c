#include "file.h"
#include "inode.h"
#include "kmalloc.h"
#include "list.h"
#include "string.h"
#include "task.h"
#include "types.h"
#include "vfs.h"

#define IOBUF_SIZE 4096

// 复制文件路径字符串

static int copy_path(char **to, const char *from) {
    MemMap_t *mm = CurrentTask->mm;
    char *buffer;
    if ((buffer = kmalloc(FS_MAX_FPATH_LEN + 1)) == NULL) {
        return -E_NOMEM;
    }
    lock_mm(mm);
    if (!copyLegalStr(mm, buffer, from, FS_MAX_FPATH_LEN + 1)) {
        unlock_mm(mm);
        goto failed_cleanup;
    }
    unlock_mm(mm);
    *to = buffer;
    return 0;

failed_cleanup:
    kfree(buffer, FS_MAX_FPATH_LEN + 1);
    return -E_INVAL;
}

/* sysfile_open - open file */
// 打开文件的系统调用实现
// 参数: 文件路径, flag
int sysfile_open(const char *__path, uint32_t open_flags) {
    // sprintk("sysfile_open:\n");
    int ret;
    char *path;
    // 为何要重新复制一次字符串?
    if ((ret = copy_path(&path, __path)) != 0) {
        return ret;
    }
    ret = file_open(path, open_flags);
    kfree(path, FS_MAX_FPATH_LEN + 1);
    return ret;
}
int sysfile_close(int fd) { return file_close(fd); }

//返回值正值为读取的字节数
int sysfile_read(int fd, void *base, size_t len) {
    // sprintk("sysfile_read:\n");
    MemMap_t *mm = CurrentTask->mm;
    if (len == 0) {
        return 0;
    }
    if (!file_test_rw(fd, 1, 0)) {
        return -E_INVAL;
    }
    // 分配一个缓冲区,即是每次读取的最大单位
    void *buffer;
    if ((buffer = kmalloc(IOBUF_SIZE)) == NULL) {
        return -E_NOMEM;
    }

    int ret = 0;
    size_t copied = 0, alen;
    // 循环每次读取一个 buffer 到内核空间
    while (len != 0) {
        if ((alen = IOBUF_SIZE) > len) {
            alen = len;
        }
        // 1. fd 读取到-->  buffer
        ret = file_read(fd, buffer, alen, &alen);
        if (alen != 0) {
            lock_mm(mm);
            {
                //  2. 复制数据到用户空间
                if (copyToUser(mm, base, buffer, alen)) {
                    base += alen, len -= alen, copied += alen;
                } //TODO
                // else if (ret == 0) {
                //     ret = -E_INVAL;
                // }
            }
            unlock_mm(mm);
        }
        if (ret != 0 || alen == 0) {
            goto out;
        }
    }

out:
    kfree(buffer, IOBUF_SIZE);
    if (copied != 0) {
        return copied;
    }
    return ret;
}

int sysfile_write(int fd, void *base, size_t len) {
    MemMap_t *mm = CurrentTask->mm;
    if (len == 0) {
        return 0;
    }
    if (!file_test_rw(fd, 0, 1)) {
        return -E_INVAL;
    }
    void *buffer;
    if ((buffer = kmalloc(IOBUF_SIZE)) == NULL) {
        return -E_NOMEM;
    }

    int ret = 0;
    size_t copied = 0, alen;
    while (len != 0) {
        if ((alen = IOBUF_SIZE) > len) {
            alen = len;
        }
        lock_mm(mm);
        {
            if (!copyFromUser(mm, buffer, base, alen, 0)) {
                ret = -E_INVAL;
            }
        }
        unlock_mm(mm);
        if (ret == 0) {
            ret = file_write(fd, buffer, alen, &alen);
            if (alen != 0) {
                base += alen, len -= alen, copied += alen;
            }
        }
        if (ret != 0 || alen == 0) {
            goto out;
        }
    }

out:
    kfree(buffer, IOBUF_SIZE);
    if (copied != 0) {
        return copied;
    }
    return ret;
}

int sysfile_seek(int fd, int32_t pos, int flag) {
    return file_seek(fd, pos, flag);
}
int sysfile_fstat(int fd, Stat_t *__stat){
    MemMap_t *mm = CurrentTask->mm;
    int ret;
    Stat_t __local_stat, *stat = &__local_stat;
    if ((ret = file_fstat(fd, stat)) != 0) {
        return ret;
    }

    lock_mm(mm);
    {
        if (!copyToUser(mm, __stat, stat, sizeof(Stat_t))) {
            ret = -E_INVAL;
        }
    }
    unlock_mm(mm);
    return ret;
}

int sysfile_fsync(int fd) { return file_fsync(fd); }
