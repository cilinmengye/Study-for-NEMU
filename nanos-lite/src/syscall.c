#include <common.h>
#include <unistd.h>
#include "syscall.h"

static void SYS_exit(Context *c){
  halt(c->GPRx);
}

static void SYS_yield(Context *c){
  yield();
  c->GPRx = 0;
}

static void SYS_write(Context *c){
  int fd = c->GPR2;
  char *buf = (char *)c->GPR3;
  size_t count = c->GPR4;
  printf("??? : %s\n", buf);
  switch (fd)
  {
  case 1:
  case 2:
    for (unsigned long i = 0; i < count; i++)
      putch(buf[i]);
    break;
  default:
    assert(0);
    break;
  }
}

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;

  switch (a[0]) {
    case (uintptr_t) 0: SYS_exit (c); break;
    case (uintptr_t) 1: SYS_yield(c); break;
    case (uintptr_t) 4: SYS_write(c); break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
}
