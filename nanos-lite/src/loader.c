#include "common.h"
#include "fs.h"

#define DEFAULT_ENTRY ((void *)0x4000000)

void ramdisk_read(void *buf, off_t offset, size_t len);
size_t get_ramdisk_size(void);

uintptr_t loader(_Protect *as, const char *filename) {
  if (filename == NULL) {
    size_t size = get_ramdisk_size();
    ramdisk_read(DEFAULT_ENTRY, 0, size);
    return (uintptr_t)DEFAULT_ENTRY;
  }

  int fd = fs_open(filename, 0, 0);
  size_t size = fs_filesz(fd);

  Log("load file: %s, size = %d", filename, size);

  fs_read(fd, DEFAULT_ENTRY, size);
  fs_close(fd);

  return (uintptr_t)DEFAULT_ENTRY;
}
