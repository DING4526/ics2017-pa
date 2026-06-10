#include "FLOAT.h"
#include <stdint.h>
#include <assert.h>

FLOAT F_mul_F(FLOAT a, FLOAT b) {
  return (FLOAT)(((int64_t)a * b) / F_SCALE);
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

  uint32_t q = a / b;
  uint32_t r = a % b;

  /*
   * a / b 的整数部分是 q。
   * 小数部分通过 r 继续放大 2^16 后除以 b。
   *
   * result = (a / b) * 2^16
   *        = q * 2^16 + r * 2^16 / b
   */
  uint32_t res = (q << F_SHIFT) + ((r << F_SHIFT) / b);

  return sign < 0 ? -(FLOAT)res : (FLOAT)res;
}

FLOAT f2F(float a) {
  /* You should figure out how to convert `a' into FLOAT without
   * introducing x87 floating point instructions. Else you can
   * not run this code in NEMU before implementing x87 floating
   * point instructions, which is contrary to our expectation.
   *
   * Hint: The bit representation of `a' is already on the
   * stack. How do you retrieve it to another variable without
   * performing arithmetic operations on it directly?
   */

  uint32_t u = *(uint32_t *)&a;

  uint32_t sign = u >> 31;
  uint32_t exp = (u >> 23) & 0xff;
  uint32_t frac = u & 0x7fffff;

  assert(exp != 0xff); // NaN or Inf is not supported

  if (exp == 0) {
    // Subnormal numbers are too small for Q16 in this PA.
    return 0;
  }

  uint64_t mant = (1u << 23) | frac;

  /*
   * float value:
   *   (-1)^sign * mant * 2^(exp - 127 - 23)
   *
   * FLOAT value should be:
   *   real_value * 2^16
   *
   * Therefore:
   *   FLOAT = mant * 2^(exp - 127 - 23 + 16)
   *         = mant * 2^(exp - 134)
   */
  int shift = (int)exp - 134;

  uint64_t val;
  if (shift >= 0) {
    val = mant << shift;
  } else {
    val = mant >> (-shift);
  }

  if (sign) {
    return -(FLOAT)val;
  } else {
    return (FLOAT)val;
  }
}

FLOAT Fabs(FLOAT a) {
  return a >= 0 ? a : -a;
}

/* Functions below are already implemented */

FLOAT Fsqrt(FLOAT x) {

  FLOAT dt, t = int2F(2);

  do {
    dt = F_div_int((F_div_F(x, t) - t), 2);
    t += dt;
  } while(Fabs(dt) > f2F(1e-4));

  return t;
}

FLOAT Fpow(FLOAT x, FLOAT y) {
  /* we only compute x^0.333 */
  FLOAT t2, dt, t = int2F(2);

  do {
    t2 = F_mul_F(t, t);
    dt = (F_div_F(x, t2) - t) / 3;
    t += dt;
  } while(Fabs(dt) > f2F(1e-4));

  return t;
}


void test_FLOAT(void) {
  assert(int2F(1) == 0x00010000);
  assert(int2F(10) == 0x000a0000);

  assert(F2int(int2F(10)) == 10);

  assert(F_mul_F(int2F(2), int2F(3)) == int2F(6));
  assert(F_div_F(int2F(6), int2F(3)) == int2F(2));

  assert(f2F(1.2f) == 0x13333);
  assert(f2F(5.6f) == 0x59999);
  assert(f2F(-1.2f) == -0x13333);
}
