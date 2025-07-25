#include <common.h>

#define STRACE 1

Context* schedule(Context *prev);

void do_syscall(Context *c);

static Context* do_event(Event e, Context* c) {
  switch (e.event) {
    case EVENT_YIELD:
      // Log("Nanos in yield"); break;
      #ifdef STRACE
      Log("Event: yield");
      #endif
      c = schedule(c); 
      break;
    case EVENT_SYSCALL:
      // #ifdef STRACE
      // Log("Event: syscall");
      // #endif
      do_syscall(c); break;
    default: panic("Unhandled event ID = %d", e.event);
  }
  return c;
}

void init_irq(void) {
  Log("Initializing interrupt/exception handler...");
  cte_init(do_event);
}
