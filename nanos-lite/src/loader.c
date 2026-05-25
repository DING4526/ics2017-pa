#include "common.h"
#include "fs.h"
#include "memory.h"

#define DEFAULT_ENTRY ((void *)0x8048000)

void ramdisk_read(void *buf, off_t offset, size_t len);
size_t get_ramdisk_size(void);

static void load_page(_Protect *as, int fd, uintptr_t va, size_t len) {
  void *pa = new_page();
  memset(pa, 0, PGSIZE);

  _map(as, (void *)va, pa);

  if (len > 0) {
    fs_read(fd, pa, len);
  }
}

uintptr_t loader(_Protect *as, const char *filename) {
  assert(as != NULL);
  assert(filename != NULL);

  int fd = fs_open(filename, 0, 0);
  size_t size = fs_filesz(fd);

  Log("load file: %s, size = %d", filename, size);

  uintptr_t va = (uintptr_t)DEFAULT_ENTRY;
  size_t left = size;

  while (left > 0) {
    size_t len = left > PGSIZE ? PGSIZE : left;
    load_page(as, fd, va, len);

    va += PGSIZE;
    left -= len;
  }

  fs_close(fd);

  return (uintptr_t)DEFAULT_ENTRY;
}
