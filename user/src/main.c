#include "stdio.h"
#include "string.h"
#include "syscall.h"
#include "ulib.h"

char getch() {
    char c = 0;
    read(STDIN, &c, 1);
    return c;
}
void writech(char c) {
    //
    write(STDOUT, &c, 1);
}

#define READLINE_BUFLEN 1024
void readline(char *out) {
    static char buf[READLINE_BUFLEN];

    int i = 0;
    while (1) {
        char c;
        if ((c = getch()) < 0) {
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
            writech(c);
            buf[i++] = c;
        } else if (c == '\b' && i > 0) {
            writech(c);
            writech(' ');
            writech(c);
            i--;
        } else if (c == '\n' || c == '\r') {
            writech(c);
            buf[i] = '\0';
            break;
        }
    }
    strcpy(out, buf);
}
int main(int argc, char *argv[]) {
    printf("shell:\n");
    static char str[READLINE_BUFLEN];
    memset(str, 0, READLINE_BUFLEN);
    while (1) {
        printf("$ ");
        readline(str);
        if (strcmp(str, "/fs0/user/hello") == 0) {
            printf("Hello, world!\n");
            continue;
        }
        if (strcmp(str, "hello") == 0) {
            printf("Hello, world!\n");
            continue;
        }
        if (strcmp(str, "ls") == 0) {
            sys_temp_ls();
            continue;
        }

        int fd;
        int ret;
        if ((fd = open(str, O_RDONLY)) > 0) {
            close(fd);
            int tid;
            if ((tid = fork()) == 0) {
                // 子进程
                EXECVE(str);
            } else {
                // 父进程
                // if (waitpid(tid, &ret) == 0) {
                // }
            }
        } else {
            printf("No such file!\n");
        }
    }

    while (1) {
    }

    // printf("Hello, world!\n");

    return 0;
}