#include "stdio.h"
#include "syscall.h"
#include "ulib.h"

void putstr(const char *str) {
    for (int i = 0; str[i] != 0; i++) {
        putch(str[i]);
    }
}
int main(int argc, char *argv[]) {
    
    int a = -1;

    //a = fork();
    //printf("Hello,world! pid:%d\n",getpid());
    //EXECVE("/fs0/user/user.out");
    printf("fork ret:%d\n",a);
    return 0;
}