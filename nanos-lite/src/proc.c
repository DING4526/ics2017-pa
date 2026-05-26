#include "proc.h"

#define MAX_NR_PROC 4

static PCB pcb[MAX_NR_PROC];
static int nr_proc = 0;
static uint32_t system_ticks = 0;
PCB *current = NULL;

uintptr_t loader(_Protect *as, const char *filename);
uintptr_t get_program_break(void);

// static int pcb_id(PCB *p) {
//   return p - pcb;
// }

void load_prog(const char *filename) {
  int i = nr_proc ++;
  assert(i < MAX_NR_PROC);

  _protect(&pcb[i].as);

  uintptr_t entry = loader(&pcb[i].as, filename);

  pcb[i].cur_brk = get_program_break();
  pcb[i].max_brk = pcb[i].cur_brk;
  pcb[i].last_run_tick = 0;

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

// _RegSet* schedule(_RegSet *prev) {
//   if (nr_proc == 0) {
//     panic("schedule: no process");
//   }

//   if (current != NULL && prev != NULL) {
//     current->tf = prev;
//   }

//   if (current == NULL) {
//     current = &pcb[0];
//   }
//   else {
//     int next = (pcb_id(current) + 1) % nr_proc;
//     current = &pcb[next];
//   }

//   _switch(&current->as);

//   return current->tf;
// }

_RegSet* schedule(_RegSet *prev) {

  if (nr_proc == 0) {
    panic("schedule: no process");
  }

  if (current != NULL && prev != NULL) {
    current->tf = prev;
  }

  system_ticks++;

  PCB *best = &pcb[0];
  uint32_t best_wait =
      system_ticks - pcb[0].last_run_tick;

  for (int i = 1; i < nr_proc; i++) {

    uint32_t wait =
        system_ticks - pcb[i].last_run_tick;

    if (wait > best_wait) {
      best = &pcb[i];
      best_wait = wait;
    }
  }

  current = best;

  current->last_run_tick = system_ticks;

  _switch(&current->as);

  return current->tf;
}