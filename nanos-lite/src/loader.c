#include "common.h"

#define DEFAULT_ENTRY ((void *)0x4000000)

void ramdisk_read(void *buf, off_t offset, size_t len);
size_t get_ramdisk_size(void);

uintptr_t loader(_Protect *as, const char *filename) {
  size_t size = get_ramdisk_size();

  Log("load program: ramdisk size = %d bytes", size);
  Log("load program: entry = %p", DEFAULT_ENTRY);

  ramdisk_read(DEFAULT_ENTRY, 0, size);

  return (uintptr_t)DEFAULT_ENTRY;
}
