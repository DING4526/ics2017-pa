#include <x86.h>
#include <klib.h>

#define PG_ALIGN __attribute((aligned(PGSIZE)))

static PDE kpdirs[NR_PDE] PG_ALIGN;
static PTE kptabs[PMEM_SIZE / PGSIZE] PG_ALIGN;
static void* (*palloc_f)();
static void (*pfree_f)(void*);

_Area segments[] = {      // Kernel memory mappings
  {.start = (void*)0,          .end = (void*)PMEM_SIZE}
};

#define NR_KSEG_MAP (sizeof(segments) / sizeof(segments[0]))

void _pte_init(void* (*palloc)(), void (*pfree)(void*)) {
  palloc_f = palloc;
  pfree_f = pfree;

  int i;

  // make all PDEs invalid
  for (i = 0; i < NR_PDE; i ++) {
    kpdirs[i] = 0;
  }

  PTE *ptab = kptabs;
  for (i = 0; i < NR_KSEG_MAP; i ++) {
    uint32_t pdir_idx = (uintptr_t)segments[i].start / (PGSIZE * NR_PTE);
    uint32_t pdir_idx_end = (uintptr_t)segments[i].end / (PGSIZE * NR_PTE);
    for (; pdir_idx < pdir_idx_end; pdir_idx ++) {
      // fill PDE
      kpdirs[pdir_idx] = (uintptr_t)ptab | PTE_P;

      // fill PTE
      PTE pte = PGADDR(pdir_idx, 0, 0) | PTE_P;
      PTE pte_end = PGADDR(pdir_idx + 1, 0, 0) | PTE_P;
      for (; pte < pte_end; pte += PGSIZE) {
        *ptab = pte;
        ptab ++;
      }
    }
  }

  set_cr3(kpdirs);
  set_cr0(get_cr0() | CR0_PG);
}

void _protect(_Protect *p) {
  PDE *updir = (PDE*)(palloc_f());
  p->ptr = updir;
  // map kernel space
  for (int i = 0; i < NR_PDE; i ++) {
    updir[i] = kpdirs[i];
  }

  p->area.start = (void*)0x8000000;
  p->area.end = (void*)0xc0000000;
}

void _release(_Protect *p) {
}

void _switch(_Protect *p) {
  set_cr3(p->ptr);
}

void _map(_Protect *p, void *va, void *pa) {
  PDE *pdir = (PDE *)p->ptr;
  uintptr_t vaddr = (uintptr_t)va;
  uintptr_t paddr = (uintptr_t)pa;

  assert(OFF(vaddr) == 0);
  assert(OFF(paddr) == 0);
  assert(vaddr >= (uintptr_t)p->area.start && vaddr < (uintptr_t)p->area.end);

  uint32_t pdir_idx = PDX(vaddr);
  uint32_t ptab_idx = PTX(vaddr);

  PTE *ptab;

  if ((pdir[pdir_idx] & PTE_P) == 0) {
    ptab = (PTE *)palloc_f();

    for (int i = 0; i < NR_PTE; i ++) {
      ptab[i] = 0;
    }

    pdir[pdir_idx] = ((uintptr_t)ptab) | PTE_P | PTE_W | PTE_U;
  }
  else {
    ptab = (PTE *)PTE_ADDR(pdir[pdir_idx]);
  }

  ptab[ptab_idx] = PTE_ADDR(paddr) | PTE_P | PTE_W | PTE_U;
}


void _unmap(_Protect *p, void *va) {
}

_RegSet *_umake(_Protect *p, _Area ustack, _Area kstack, void *entry, char *const argv[], char *const envp[]) {
  _RegSet *tf = (_RegSet *)((uintptr_t)kstack.end - sizeof(_RegSet));
  memset(tf, 0, sizeof(_RegSet));

  tf->eip = (uintptr_t)entry;
  tf->cs = KSEL(SEG_KCODE);
  tf->eflags = FL_IF | 0x2;
  tf->esp = (uintptr_t)ustack.end - 32;

  tf->irq = 0;
  tf->error_code = 0;

  return tf;
}
