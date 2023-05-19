#ifndef __ULIB_H
#define __ULIB_H

#include "syscall.h"
#include "types.h"

#define O_RDONLY 0 // open for reading only
#define O_WRONLY 1 // open for writing only
#define O_RDWR 2   // open for reading and writing

#define O_CREAT 0x00000004  // create file if it does not exist
#define O_EXCL 0x00000008   // error if O_CREAT and the file exists
#define O_TRUNC 0x00000010  // truncate file upon open,不可与只读共存
#define O_APPEND 0x00000020 // append on each write

extern int stdinfd;
extern int stdoutfd;
#define STDIN (stdinfd)
#define STDOUT (stdoutfd)

void exit(int err_code);
int fork(void);
int wait(void);
int waitpid(int pid, int *store);
int getpid(void);
int putch(char c);
int exec(const char *name, int argc, const char **argv);
int exec2(const char *name, const char **argv);
#define __EXECVE(name, path, ...)                                              \
    ({                                                                         \
        const char *argv[] = {path, ##__VA_ARGS__, NULL};                      \
        exec2(name, argv);                                                     \
    })

#define EXECVE(x, ...) __EXECVE(x, x, ##__VA_ARGS__)

int open(const char *path, uint32_t open_flags);
int close(int fd);
int read(int fd, void *base, size_t len);
int write(int fd, void *base, size_t len);
int seek(int fd, int32_t pos, int whence);
int fstat(int fd, Stat_t *stat);
int fsync(int fd);

#endif