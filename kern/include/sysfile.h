#ifndef __SYSFILE_H_
#define __SYSFILE_H_

#include "types.h"

struct Stat_t;
typedef struct Stat_t Stat_t;

int sysfile_open(const char *__path, uint32_t open_flags);
int sysfile_close(int fd);
int sysfile_read(int fd, void *base, size_t len);
int sysfile_write(int fd, void *base, size_t len);
int sysfile_seek(int fd, int32_t pos, int flag);
int sysfile_fstat(int fd, Stat_t *__stat);
int sysfile_fsync(int fd);

#endif