#include "ulib.h"
#include "syscall.h"

void exit(int err_code) {
    sys_exit(err_code);
    // TODO
    while (1) {
    }
}
//子进程中返回0 父进程中返回子进程tid
int fork(void) {
    //
    return sys_fork();
}
int wait(void) {
    //
    return sys_wait(0, NULL);
}
int waitpid(int pid, int *store) {
    //
    return sys_wait(pid, store);
}
int getpid(void) {
    //
    return sys_getpid();
}
int putch(char c) {
    //
    return sys_putc(c);
}
int exec(const char *name, int argc, const char **argv) {
    return sys_exec(name, argc, argv);
}
int exec2(const char *name, const char **argv) {
    int argc = 0;
    while (argv[argc] != NULL) {
        argc++;
    }
    return sys_exec(name, argc, argv);
}

int open(const char *path, uint32_t open_flags) {
    return sys_open(path, open_flags);
}

int close(int fd) {
    //
    return sys_close(fd);
}

int read(int fd, void *base, size_t len) {
    //
    return sys_read(fd, base, len);
}

int write(int fd, void *base, size_t len) {
    //
    return sys_write(fd, base, len);
}

int seek(int fd, int32_t pos, int whence) {
    //
    return sys_seek(fd, pos, whence);
}

int fstat(int fd, Stat_t *stat) {
    //
    return sys_fstat(fd, stat);
}

int fsync(int fd) {
    //
    return sys_fsync(fd);
}