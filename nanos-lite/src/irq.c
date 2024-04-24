#include <common.h>

void do_syscall(Context *c);

static Context* do_event(Event e, Context* c) {
  printf("start do_event a0:%x\n", c->GPRx);
  switch (e.event) {
    case EVENT_YIELD:
      Log("Nanos in yield"); break;
    case EVENT_SYSCALL:
      do_syscall(c); break;
    default: panic("Unhandled event ID = %d", e.event);
  }
  printf("end do_event a0:%x\n", c->GPRx);
  return c;
}

void init_irq(void) {
  Log("Initializing interrupt/exception handler...");
  cte_init(do_event);
}
