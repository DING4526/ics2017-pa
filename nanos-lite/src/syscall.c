#include "common.h"
#include "syscall.h"

_RegSet* do_syscall(_RegSet *r) {
  uintptr_t a[4];

  a[0] = SYSCALL_ARG1(r);
  a[1] = SYSCALL_ARG2(r);
  a[2] = SYSCALL_ARG3(r);
  a[3] = SYSCALL_ARG4(r);

  Log("syscall id = %d", a[0]);

  switch (a[0]) {
    case SYS_none:
      r->eax = 0;
      break;

    case SYS_exit:
      Log("SYS_exit status = %d", a[1]);
      _halt(0);
      break;

    default:
      panic("Unhandled syscall ID = %d", a[0]);
  }

  return r;
}
