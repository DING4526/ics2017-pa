#include "nemu.h"
#include <stdlib.h>
#include <time.h>

CPU_state cpu;

const char *regsl[] = {"eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"};
const char *regsw[] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};
const char *regsb[] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};

void rtl_test() {
  rtlreg_t x, y;

  x = 0x80;
  rtl_sext(&y, &x, 1);
  assert(y == 0xffffff80);

  x = 0x8000;
  rtl_sext(&y, &x, 2);
  assert(y == 0xffff8000);

  x = 0x7f;
  rtl_sext(&y, &x, 1);
  assert(y == 0x7f);

  x = 0;
  rtl_update_ZF(&x, 4);
  assert(cpu.ZF == 1);

  x = 1;
  rtl_update_ZF(&x, 4);
  assert(cpu.ZF == 0);

  x = 0x80;
  rtl_update_SF(&x, 1);
  assert(cpu.SF == 1);

  x = 0x7f;
  rtl_update_SF(&x, 1);
  assert(cpu.SF == 0);

  x = 1;
  rtl_set_CF(&x);
  rtl_get_CF(&y);
  assert(y == 1);

  x = 0;
  rtl_set_CF(&x);
  rtl_get_CF(&y);
  assert(y == 0);

  cpu.esp = 0x100000;

  x = 0x12345678;
  rtl_push(&x);
  assert(cpu.esp == 0xffffc);

  rtl_pop(&y);
  assert(y == 0x12345678);
  assert(cpu.esp == 0x100000);
}

void reg_test() {
  srand(time(0));
  uint32_t sample[8];
  uint32_t eip_sample = rand();
  cpu.eip = eip_sample;

  int i;
  for (i = R_EAX; i <= R_EDI; i ++) {
    sample[i] = rand();
    reg_l(i) = sample[i];
    assert(reg_w(i) == (sample[i] & 0xffff));
  }

  assert(reg_b(R_AL) == (sample[R_EAX] & 0xff));
  assert(reg_b(R_AH) == ((sample[R_EAX] >> 8) & 0xff));
  assert(reg_b(R_BL) == (sample[R_EBX] & 0xff));
  assert(reg_b(R_BH) == ((sample[R_EBX] >> 8) & 0xff));
  assert(reg_b(R_CL) == (sample[R_ECX] & 0xff));
  assert(reg_b(R_CH) == ((sample[R_ECX] >> 8) & 0xff));
  assert(reg_b(R_DL) == (sample[R_EDX] & 0xff));
  assert(reg_b(R_DH) == ((sample[R_EDX] >> 8) & 0xff));

  assert(sample[R_EAX] == cpu.eax);
  assert(sample[R_ECX] == cpu.ecx);
  assert(sample[R_EDX] == cpu.edx);
  assert(sample[R_EBX] == cpu.ebx);
  assert(sample[R_ESP] == cpu.esp);
  assert(sample[R_EBP] == cpu.ebp);
  assert(sample[R_ESI] == cpu.esi);
  assert(sample[R_EDI] == cpu.edi);

  assert(eip_sample == cpu.eip);

  // 额外的测试代码，测试一下RTL指令是否正确
  rtl_test();
}
