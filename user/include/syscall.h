#ifndef __SYSCALL_H_
#define __SYSCALL_H_

#include "types.h"

int syscall(int num, ...);

int sys_exit(int error_code);
int sys_fork(void);
int sys_wait(int pid, int *store);
int sys_getpid(void);
int sys_putc(int c);
int sys_exec(const char *name, int argc, const char **argv);
int sys_open(const char *path, uint32_t open_flags);
int sys_close(int fd);
int sys_read(int fd, void *base, size_t len);
int sys_write(int fd, void *base, size_t len);
int sys_seek(int fd, int32_t pos, int whence);
int sys_fstat(int fd, Stat_t *stat);
int sys_fsync(int fd);

#endif