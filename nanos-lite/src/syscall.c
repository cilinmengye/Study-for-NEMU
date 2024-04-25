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
  size_t i = 0;
  switch (fd)
  {
  case 1:
  case 2:
    for (i = 0; i < count; i++)
      putch(buf[i]);
    c->GPRx = i;
    break;
  default:
    assert(0);
    c->GPRx = -1;
    break;
  }
}

static void SYS_brk(Context *c){

  c->GPRx = 0;
}

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;
  printf("do_syscall: %d\n", (int)a[0]);
  switch (a[0]) {
    case (uintptr_t) 0: SYS_exit (c); break;
    case (uintptr_t) 1: SYS_yield(c); break;
    case (uintptr_t) 4: SYS_write(c); break;
    case (uintptr_t) 9: SYS_brk(c);   break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
}
