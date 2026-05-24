#include "common.h"

#define NAME(key) \
  [_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [_KEY_NONE] = "NONE",
  _KEYS(NAME)
};

size_t events_read(void *buf, size_t len) {
  static char event[64];
  static int event_len = 0;
  static int event_pos = 0;

  if (len == 0) {
    return 0;
  }

  if (event_pos >= event_len) {
    int key = _read_key();

    if (key != _KEY_NONE) {
      int is_down = (key & 0x8000) != 0;
      int keycode = key & 0x7fff;

      if (keycode > _KEY_NONE && keycode < 256 && keyname[keycode] != NULL) {
        event_len = sprintf(event, "%s %s\n",
            is_down ? "kd" : "ku",
            keyname[keycode]);
      }
      else {
        event_len = 0;
      }
    }
    else {
      event_len = sprintf(event, "t %d\n", (int)_uptime());
    }

    event_pos = 0;
  }

  size_t n = event_len - event_pos;
  if (n > len) {
    n = len;
  }

  memcpy(buf, event + event_pos, n);
  event_pos += n;

  return n;
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
