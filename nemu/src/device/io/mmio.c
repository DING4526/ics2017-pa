#include "common.h"
#include "device/mmio.h"

#define MMIO_SPACE_MAX (512 * 1024)
#define NR_MAP 8

#define MMIO_PAGE_SHIFT 12
#define MMIO_PAGE_SIZE  (1 << MMIO_PAGE_SHIFT)

/*
 * NEMU 的物理内存大小是 128MB。
 * 128MB / 4KB = 32768 pages.
 *
 * 注意：如果某个 MMIO 地址超过 128MB，我们不能直接根据 mmio_page[]
 * 判断它不是 MMIO，而是继续走 maps[] 遍历。
 */
#define PMEM_SIZE_FOR_MMIO_FILTER (128 * 1024 * 1024)
#define MMIO_PAGE_NR (PMEM_SIZE_FOR_MMIO_FILTER >> MMIO_PAGE_SHIFT)

static uint8_t mmio_space_pool[MMIO_SPACE_MAX];
static uint32_t mmio_space_free_index = 0;

typedef struct {
  paddr_t low;
  paddr_t high;
  uint8_t *mmio_space;
  mmio_callback_t callback;
} MMIO_t;

static MMIO_t maps[NR_MAP];
static int nr_map = 0;

/*
 * 页级快速过滤表：
 * mmio_page[p] == true 表示第 p 个 4KB 页里可能有 MMIO。
 * false 表示这个页一定不是 MMIO，可以直接返回 -1。
 */
static bool mmio_page[MMIO_PAGE_NR];

/*
 * last-hit cache:
 * framebuffer 连续写入时通常会连续命中同一个 MMIO map。
 */
static int last_mmio = -1;

/* device interface */
void* add_mmio_map(paddr_t addr, int len, mmio_callback_t callback) {
  assert(nr_map < NR_MAP);
  assert(mmio_space_free_index + len <= MMIO_SPACE_MAX);

  uint8_t *space_base = &mmio_space_pool[mmio_space_free_index];

  maps[nr_map].low = addr;
  maps[nr_map].high = addr + len - 1;
  maps[nr_map].mmio_space = space_base;
  maps[nr_map].callback = callback;

  /*
   * 标记这个 MMIO 区间覆盖到的页。
   * 只标记 128MB 范围内的页；超过这个范围的地址在 is_mmio()
   * 中仍然会走 maps[] 遍历，不会被误判。
   */
  paddr_t low_page = addr >> MMIO_PAGE_SHIFT;
  paddr_t high_page = (addr + len - 1) >> MMIO_PAGE_SHIFT;

  for (paddr_t p = low_page; p <= high_page && p < MMIO_PAGE_NR; p ++) {
    mmio_page[p] = true;
  }

  nr_map ++;
  mmio_space_free_index += len;

  return space_base;
}

/* bus interface */
int is_mmio(paddr_t addr) {
  paddr_t page = addr >> MMIO_PAGE_SHIFT;

  /*
   * 如果地址在 128MB 物理内存范围内，而且所在页没有被标记为 MMIO，
   * 那它一定不是 MMIO。
   *
   * 如果 page >= MMIO_PAGE_NR，说明它超过 128MB。
   * 这种情况不能直接返回 -1，因为可能存在高地址 MMIO。
   */
  if (page < MMIO_PAGE_NR && !mmio_page[page]) {
    return -1;
  }

  /*
   * 对真实 MMIO 连续访问做快速命中。
   */
  if (last_mmio != -1) {
    MMIO_t *m = &maps[last_mmio];
    if (addr >= m->low && addr <= m->high) {
      return last_mmio;
    }
  }

  for (int i = 0; i < nr_map; i ++) {
    if (addr >= maps[i].low && addr <= maps[i].high) {
      last_mmio = i;
      return i;
    }
  }

  return -1;
}

uint32_t mmio_read(paddr_t addr, int len, int map_NO) {
  assert(len >= 1 && len <= 4);

  MMIO_t *map = &maps[map_NO];

  uint32_t data = *(uint32_t *)(map->mmio_space + (addr - map->low))
    & (~0u >> ((4 - len) << 3));

  map->callback(addr, len, false);

  return data;
}

void mmio_write(paddr_t addr, int len, uint32_t data, int map_NO) {
  assert(len >= 1 && len <= 4);

  MMIO_t *map = &maps[map_NO];

  uint8_t *p = map->mmio_space + (addr - map->low);
  uint8_t *p_data = (uint8_t *)&data;

  switch (len) {
    case 4: p[3] = p_data[3];
    case 3: p[2] = p_data[2];
    case 2: p[1] = p_data[1];
    case 1: p[0] = p_data[0]; break;
  }

  map->callback(addr, len, true);
}
