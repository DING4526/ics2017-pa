#include "nemu.h"
#include "device/mmio.h"
#include "memory/mmu.h"

#define PMEM_SIZE (128 * 1024 * 1024)

#define pmem_rw(addr, type) *(type *)({\
    Assert(addr < PMEM_SIZE, "physical address(0x%08x) is out of bound", addr); \
    guest_to_host(addr); \
    })

uint8_t pmem[PMEM_SIZE];

/* Memory accessing interfaces */

uint32_t paddr_read(paddr_t addr, int len) {
#ifdef HAS_IOE
  int map_no = is_mmio(addr);
  if (map_no != -1) {
    return mmio_read(addr, len, map_no);
  }
#endif

  return pmem_rw(addr, uint32_t) & (~0u >> ((4 - len) << 3));
}

void paddr_write(paddr_t addr, int len, uint32_t data) {
#ifdef HAS_IOE
  int map_no = is_mmio(addr);
  if (map_no != -1) {
    mmio_write(addr, len, data, map_no);
    return;
  }
#endif

  memcpy(guest_to_host(addr), &data, len);
}

static inline uint32_t pdir_idx(vaddr_t addr) {
  return (addr >> 22) & 0x3ff;
}

static inline uint32_t ptab_idx(vaddr_t addr) {
  return (addr >> 12) & 0x3ff;
}

static inline uint32_t page_off(vaddr_t addr) {
  return addr & PAGE_MASK;
}

static paddr_t page_translate(vaddr_t addr, bool is_write) {
  paddr_t pdir_base = cpu.cr3.page_directory_base << 12;

  paddr_t pde_addr = pdir_base + pdir_idx(addr) * sizeof(PDE);
  PDE pde;
  pde.val = paddr_read(pde_addr, 4);

  Assert(pde.present,
      "page fault: PDE not present, vaddr=0x%08x cr3=0x%08x pde_addr=0x%08x pde=0x%08x",
      addr, cpu.cr3.val, pde_addr, pde.val);

  if (!pde.accessed) {
    pde.accessed = 1;
    paddr_write(pde_addr, 4, pde.val);
  }

  paddr_t ptab_base = pde.page_frame << 12;
  paddr_t pte_addr = ptab_base + ptab_idx(addr) * sizeof(PTE);
  PTE pte;
  pte.val = paddr_read(pte_addr, 4);

  Assert(pte.present,
      "page fault: PTE not present, vaddr=0x%08x pde=0x%08x pte_addr=0x%08x pte=0x%08x",
      addr, pde.val, pte_addr, pte.val);

  bool need_writeback = false;

  if (!pte.accessed) {
    pte.accessed = 1;
    need_writeback = true;
  }

  if (is_write && !pte.dirty) {
    pte.dirty = 1;
    need_writeback = true;
  }

  if (need_writeback) {
    paddr_write(pte_addr, 4, pte.val);
  }

  return (pte.page_frame << 12) | page_off(addr);
}

uint32_t vaddr_read(vaddr_t addr, int len) {
  assert(len == 1 || len == 2 || len == 4);

  if (!cpu.cr0.paging) {
    return paddr_read(addr, len);
  }

  Assert(page_off(addr) + len <= PAGE_SIZE,
      "vaddr_read across page boundary: addr=0x%08x len=%d", addr, len);

  return paddr_read(page_translate(addr, false), len);
}

void vaddr_write(vaddr_t addr, int len, uint32_t data) {
  assert(len == 1 || len == 2 || len == 4);

  if (!cpu.cr0.paging) {
    paddr_write(addr, len, data);
    return;
  }

  Assert(page_off(addr) + len <= PAGE_SIZE,
      "vaddr_write across page boundary: addr=0x%08x len=%d", addr, len);

  paddr_write(page_translate(addr, true), len, data);
}
