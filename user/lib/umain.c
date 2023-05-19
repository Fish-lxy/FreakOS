#include "syscall.h"
#include "types.h"
#include "ulib.h"

int stdinfd = -1;
int stdoutfd = -1;

static int initfd(int fd2, const char *path, uint32_t open_flags) {
    int fd1, ret;
    if ((fd1 = sys_open(path, open_flags)) < 0) {
        return fd1;
    }
    if (fd1 != fd2) {
        sys_close(fd2);
        // ret = dup2(fd1, fd2);
        sys_close(fd1);
    }
    return ret;
}

void umain(int argc, char *argv[]) {
    stdinfd = sys_open("/stdin", O_RDONLY);
    stdoutfd = sys_open("/stdout", O_WRONLY);
    //putch('0' + stdoutfd);
    int ret = main(argc, argv);
    exit(0);
}