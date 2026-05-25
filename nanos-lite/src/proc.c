#include "proc.h"

#define MAX_NR_PROC 4

static PCB pcb[MAX_NR_PROC];
static int nr_proc = 0;
PCB *current = NULL;

uintptr_t loader(_Protect *as, const char *filename);
uintptr_t get_program_break(void);

void load_prog(const char *filename) {
  int i = nr_proc ++;
  assert(i < MAX_NR_PROC);

  _protect(&pcb[i].as);

  uintptr_t entry = loader(&pcb[i].as, filename);

  pcb[i].cur_brk = get_program_break();
  pcb[i].max_brk = pcb[i].cur_brk;

  _Area stack;
  stack.start = pcb[i].stack;
  stack.end = stack.start + sizeof(pcb[i].stack);

  pcb[i].tf = _umake(&pcb[i].as, stack, stack, (void *)entry, NULL, NULL);

  // TODO: remove the following temporary direct-jump code after _umake() works.
  uintptr_t ustack_top = (uintptr_t)pcb[i].as.area.end;
  for (uintptr_t va = ustack_top - STACK_SIZE; va < ustack_top; va += PGSIZE) {
    void *pa = new_page();
    memset(pa, 0, PGSIZE);
    _map(&pcb[i].as, (void *)va, pa);
  }

  _switch(&pcb[i].as);
  current = &pcb[i];

  asm volatile(
      "movl %0, %%esp; "
      "jmp *%1"
      :
      : "r"(ustack_top), "r"(entry)
      : "memory"
  );
}

_RegSet* schedule(_RegSet *prev) {
  return NULL;
}
