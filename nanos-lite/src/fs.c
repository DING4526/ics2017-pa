#include "fs.h"

typedef struct {
  char *name;
  size_t size;
  off_t disk_offset;
} Finfo;

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_FB, FD_EVENTS, FD_DISPINFO, FD_NORMAL};

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
  {"stdin (note that this is not the actual stdin)", 0, 0},
  {"stdout (note that this is not the actual stdout)", 0, 0},
  {"stderr (note that this is not the actual stderr)", 0, 0},
  [FD_FB] = {"/dev/fb", 0, 0},
  [FD_EVENTS] = {"/dev/events", 0, 0},
  [FD_DISPINFO] = {"/proc/dispinfo", 128, 0},
#include "files.h"
};

#define NR_FILES (sizeof(file_table) / sizeof(file_table[0]))

void init_fs() {
  // TODO: initialize the size of /dev/fb
}

size_t fs_write(int fd, const void *buf, size_t len) {
  if (fd == FD_STDOUT || fd == FD_STDERR) {
    const char *p = buf;
    for (size_t i = 0; i < len; i ++) {
      _putc(p[i]);
    }
    return len;
  }

  panic("fs_write: unsupported fd = %d", fd);
  return -1;
}
