#include "cpu/exec.h"

static inline void clear_CF_OF() {
  rtl_li(&t0, 0);
  rtl_set_CF(&t0);
  rtl_set_OF(&t0);
}

make_EHelper(test) {
  rtl_and(&t2, &id_dest->val, &id_src->val);

  rtl_update_ZFSF(&t2, id_dest->width);
  clear_CF_OF();

  print_asm_template2(test);
}

make_EHelper(and) {
  rtl_and(&t2, &id_dest->val, &id_src->val);
  operand_write(id_dest, &t2);

  rtl_update_ZFSF(&t2, id_dest->width);
  clear_CF_OF();

  print_asm_template2(and);
}

make_EHelper(xor) {
  rtl_xor(&t2, &id_dest->val, &id_src->val);
  operand_write(id_dest, &t2);

  rtl_update_ZFSF(&t2, id_dest->width);
  clear_CF_OF();

  print_asm_template2(xor);
}

make_EHelper(or) {
  rtl_or(&t2, &id_dest->val, &id_src->val);
  operand_write(id_dest, &t2);

  rtl_update_ZFSF(&t2, id_dest->width);
  clear_CF_OF();

  print_asm_template2(or);
}


make_EHelper(sar) {
  rtl_andi(&t1, &id_src->val, 0x1f);

  rtl_sext(&t0, &id_dest->val, id_dest->width);
  rtl_sar(&t2, &t0, &t1);
  operand_write(id_dest, &t2);

  rtl_update_ZFSF(&t2, id_dest->width);
  // unnecessary to update CF and OF in NEMU

  print_asm_template2(sar);
}

make_EHelper(shl) {
  rtl_andi(&t1, &id_src->val, 0x1f);

  rtl_shl(&t2, &id_dest->val, &t1);
  operand_write(id_dest, &t2);

  rtl_update_ZFSF(&t2, id_dest->width);
  // unnecessary to update CF and OF in NEMU

  print_asm_template2(shl);
}

make_EHelper(shr) {
  rtl_andi(&t1, &id_src->val, 0x1f);

  rtl_shr(&t2, &id_dest->val, &t1);
  operand_write(id_dest, &t2);

  rtl_update_ZFSF(&t2, id_dest->width);
  // unnecessary to update CF and OF in NEMU

  print_asm_template2(shr);
}

make_EHelper(setcc) {
  uint8_t subcode = decoding.opcode & 0xf;
  rtl_setcc(&t2, subcode);
  operand_write(id_dest, &t2);

  print_asm("set%s %s", get_cc_name(subcode), id_dest->str);
}

make_EHelper(not) {
  rtl_not(&id_dest->val);
  operand_write(id_dest, &id_dest->val);

  print_asm_template1(not);
}

make_EHelper(rol) {
  rtl_andi(&t1, &id_src->val, 0x1f);

  if (t1 != 0) {
    if (id_dest->width == 1) {
      t1 %= 8;
      t0 = id_dest->val & 0xff;
      t2 = ((t0 << t1) | (t0 >> (8 - t1))) & 0xff;
    }
    else if (id_dest->width == 2) {
      t1 %= 16;
      t0 = id_dest->val & 0xffff;
      t2 = ((t0 << t1) | (t0 >> (16 - t1))) & 0xffff;
    }
    else {
      t0 = id_dest->val;
      t2 = (t0 << t1) | (t0 >> (32 - t1));
    }

    operand_write(id_dest, &t2);
  }

  print_asm_template2(rol);
}