#include "cpu/exec.h"
#include "memory/mmu.h"

#define IRQ_TIMER 32

static volatile bool intr_pending = false;

void raise_intr(uint8_t NO, vaddr_t ret_addr) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * That is, use ``NO'' to index the IDT.
   */

  rtl_push(&cpu.eflags);
  rtl_push(&cpu.cs);
  rtl_push(&ret_addr);

  cpu.IF = 0;

  vaddr_t idt_entry = cpu.idtr.base + NO * 8;

  uint32_t lo = vaddr_read(idt_entry, 4);
  uint32_t hi = vaddr_read(idt_entry + 4, 4);

  vaddr_t target = (lo & 0x0000ffff) | (hi & 0xffff0000);

  decoding.is_jmp = 1;
  decoding.jmp_eip = target;

  // printf("raise_intr: NO = 0x%x, idt_base = 0x%x, idt_addr = 0x%x, target = 0x%x, ret = 0x%x\n",
  //   NO, cpu.idtr.base, idt_entry, target, ret_addr);
}

void dev_raise_intr() {
  intr_pending = true;
}

void query_intr() {
  if (intr_pending && cpu.IF) {
    intr_pending = false;

    raise_intr(IRQ_TIMER, cpu.eip);

    cpu.eip = decoding.jmp_eip;
    decoding.is_jmp = 0;
  }
}
