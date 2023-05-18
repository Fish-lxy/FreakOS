#ifndef __SYSFILE_H_
#define __SYSFILE_H_

#include "types.h"

int sysfile_open(const char *__path, uint32_t open_flags);
int sysfile_close(int fd);
int sysfile_read(int fd, void *base, size_t len);
int sysfile_write(int fd, void *base, size_t len);
int sysfile_seek(int fd, int32_t pos, int flag);
int sysfile_fsync(int fd);

#endif