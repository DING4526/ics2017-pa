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

  _Area ustack;
  ustack.start = pcb[i].as.area.end - STACK_SIZE;
  ustack.end = pcb[i].as.area.end;

  for (uintptr_t va = (uintptr_t)ustack.start; va < (uintptr_t)ustack.end; va += PGSIZE) {
    void *pa = new_page();
    memset(pa, 0, PGSIZE);
    _map(&pcb[i].as, (void *)va, pa);
  }

  _Area kstack;
  kstack.start = pcb[i].stack;
  kstack.end = kstack.start + sizeof(pcb[i].stack);

  pcb[i].tf = _umake(&pcb[i].as, ustack, kstack, (void *)entry, NULL, NULL);

  Log("load_prog: %s entry=%p ustack=[%p,%p) tf=%p",
      filename, entry, ustack.start, ustack.end, pcb[i].tf);
}

_RegSet* schedule(_RegSet *prev) {
  if (current != NULL && prev != NULL) {
    current->tf = prev;
  }

  if (nr_proc == 0) {
    panic("schedule: no process");
  }

  if (current == NULL) {
    current = &pcb[0];
  }

  _switch(&current->as);

  return current->tf;
}

