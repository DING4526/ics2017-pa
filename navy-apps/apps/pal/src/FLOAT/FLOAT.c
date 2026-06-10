#include "FLOAT.h"
#include <stdint.h>
#include <assert.h>
#include <stdio.h>

FLOAT F_mul_F(FLOAT a, FLOAT b) {
  int sign = 1;

  if (a < 0) {
    a = -a;
    sign = -sign;
  }
  if (b < 0) {
    b = -b;
    sign = -sign;
  }

  uint32_t ua = (uint32_t)a;
  uint32_t ub = (uint32_t)b;

  uint32_t a_hi = ua >> F_SHIFT;
  uint32_t a_lo = ua & (F_SCALE - 1);
  uint32_t b_hi = ub >> F_SHIFT;
  uint32_t b_lo = ub & (F_SCALE - 1);

  uint32_t res =
      (a_hi * b_hi << F_SHIFT)
    + (a_hi * b_lo)
    + (a_lo * b_hi)
    + ((a_lo * b_lo) >> F_SHIFT);

  return sign < 0 ? -(FLOAT)res : (FLOAT)res;
}

FLOAT F_div_F(FLOAT a, FLOAT b) {
  assert(b != 0);

  int sign = 1;

  if (a < 0) {
    a = -a;
    sign = -sign;
  }
  if (b < 0) {
    b = -b;
    sign = -sign;
  }

  uint32_t ua = (uint32_t)a;
  uint32_t ub = (uint32_t)b;

  uint32_t int_part = ua / ub;
  uint32_t rem = ua % ub;

  uint32_t frac_part = 0;

  for (int i = 0; i < F_SHIFT; i ++) {
    rem <<= 1;
    frac_part <<= 1;

    if (rem >= ub) {
      rem -= ub;
      frac_part |= 1;
    }
  }

  uint32_t res = (int_part << F_SHIFT) | frac_part;

  return sign < 0 ? -(FLOAT)res : (FLOAT)res;
}

FLOAT f2F(float a) {
  union {
    float f;
    uint32_t u;
  } conv;

  conv.f = a;
  uint32_t u = conv.u;

  uint32_t sign = u >> 31;
  uint32_t exp = (u >> 23) & 0xff;
  uint32_t frac = u & 0x7fffff;

  assert(exp != 0xff);

  if (exp == 0) {
    return 0;
  }

  uint32_t mant = (1u << 23) | frac;

  int shift = (int)exp - 134;

  uint32_t val;
  if (shift >= 0) {
    val = mant << shift;
  } else {
    val = mant >> (-shift);
  }

  return sign ? -(FLOAT)val : (FLOAT)val;
}

FLOAT Fabs(FLOAT a) {
  return a >= 0 ? a : -a;
}

/* Functions below are already implemented */

FLOAT Fsqrt(FLOAT x) {
  FLOAT dt, t = int2F(2);
  int iter = 0;

  do {
    dt = F_div_int((F_div_F(x, t) - t), 2);
    t += dt;

    iter ++;
    if (iter > 100) {
      printf("[FLOAT] Fsqrt stuck: x=0x%x t=0x%x dt=0x%x\n", x, t, dt);
      assert(0);
    }
  } while(Fabs(dt) > f2F(1e-4));

  return t;
}

FLOAT Fpow(FLOAT x, FLOAT y) {
  /* we only compute x^0.333 */
  FLOAT t2, dt, t = int2F(2);
  int iter = 0;

  do {
    t2 = F_mul_F(t, t);
    dt = (F_div_F(x, t2) - t) / 3;
    t += dt;

    iter ++;
    if (iter > 100) {
      printf("[FLOAT] Fpow stuck: x=0x%x y=0x%x t=0x%x t2=0x%x dt=0x%x\n",
          x, y, t, t2, dt);
      assert(0);
    }
  } while(Fabs(dt) > f2F(1e-4));

  return t;
}

static int abs_int(int x) {
  return x < 0 ? -x : x;
}

void test_FLOAT(void) {
  static int tested = 0;
  if (tested) {
    return;
  }
  tested = 1;
  assert(int2F(1) == 0x00010000);
  assert(int2F(10) == 0x000a0000);

  assert(F2int(int2F(10)) == 10);

  assert(F_mul_F(int2F(2), int2F(3)) == int2F(6));
  assert(F_div_F(int2F(6), int2F(3)) == int2F(2));

  assert(abs_int(f2F(1.2f) - 0x13333) <= 1);
  assert(abs_int(f2F(5.6f) - 0x59999) <= 1);
  assert(abs_int(f2F(-1.2f) + 0x13333) <= 1);

  assert(abs_int(F_mul_F(f2F(1.5f), f2F(1.5f)) - f2F(2.25f)) <= 2);
  assert(abs_int(F_div_F(f2F(1.0f), f2F(3.0f)) - f2F(0.333333f)) <= 2);
  assert(abs_int(F_div_F(f2F(10.0f), f2F(4.0f)) - f2F(2.5f)) <= 2);

  FLOAT cube = Fpow(int2F(8), f2F(0.333f));
  assert(abs_int(cube - int2F(2)) <= 5);

  FLOAT root = Fsqrt(int2F(4));
  assert(abs_int(root - int2F(2)) <= 5);

  printf("[PA5] FLOAT test passed.\n");
}
