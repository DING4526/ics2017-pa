#include "common.h"

#define NAME(key) \
  [_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [_KEY_NONE] = "NONE",
  _KEYS(NAME)
};

size_t events_read(void *buf, size_t len) {
  int key = _read_key();

  if (key != _KEY_NONE) {
    int is_down = (key & 0x8000) != 0;
    int keycode = key & 0x7fff;

    return snprintf(buf, len, "%s %s\n",
        is_down ? "kd" : "ku",
        keyname[keycode]);
  }

  return snprintf(buf, len, "t %u\n", _uptime());
}

static char dispinfo[128] __attribute__((used));

size_t dispinfo_read(void *buf, off_t offset, size_t len) {
  size_t size = strlen(dispinfo);

  if (offset >= size) {
    return 0;
  }

  if (offset + len > size) {
    len = size - offset;
  }

  memcpy(buf, dispinfo + offset, len);
  return len;
}

size_t fb_write(const void *buf, off_t offset, size_t len) {
  int pixel_offset = offset / sizeof(uint32_t);
  int x = pixel_offset % _screen.width;
  int y = pixel_offset / _screen.width;

  int pixels = len / sizeof(uint32_t);
  const uint32_t *p = buf;

  while (pixels > 0) {
    int w = _screen.width - x;
    if (w > pixels) {
      w = pixels;
    }

    _draw_rect(p, x, y, w, 1);

    p += w;
    pixels -= w;
    x = 0;
    y ++;
  }

  return len;
}

void init_device() {
  _ioe_init();

  // TODO: print the string to array `dispinfo` with the format
  // described in the Navy-apps convention
  snprintf(dispinfo, sizeof(dispinfo),
    "WIDTH:%d\nHEIGHT:%d\n", _screen.width, _screen.height);
}
