#ifndef __FS_H__
#define __FS_H__

#include "common.h"

enum {SEEK_SET, SEEK_CUR, SEEK_END};

size_t fs_write(int fd, const void *buf, size_t len);

#endif
