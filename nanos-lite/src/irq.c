#include <common.h>

Context* schedule(Context *prev);

#ifdef STRACE
extern int classify_PCB();
#endif


void do_syscall(Context *c);

static Context* do_event(Event e, Context* c) {
  switch (e.event) {
    case EVENT_YIELD:
      // Log("Nanos in yield"); break;
      #ifdef STRACE
      int fpcb = classify_PCB();
      c = schedule(c);
      int tpcb = classify_PCB();
      Log("Event: yield, Switching Processes from pcb[%d] to pcb[%d]", fpcb, tpcb);
      // Log("context page dir address is %p", c->pdir);
      #else 
      c = schedule(c); 
      #endif
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
