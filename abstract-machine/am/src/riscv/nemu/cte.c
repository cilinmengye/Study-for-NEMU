#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>

static Context* (*user_handler)(Event, Context*) = NULL;

// static void debug(uint32_t bit){
//     uint32_t i = 1 << 31;
//     uint32_t j = 31;
//     while (i){
//       uint32_t k = bit & i;
//       k = k >> j;
//       printf("%d",k);
//       i = i >> 1;
//       j--;
//     }
//     printf("\t0x%x\n", bit);
// }

// static void debugContext(Context *c){
//     for (int i = 0; i < 32; i++)
//       debug(c->gpr[i]);
//     printf("c->mcause: "); debug(c->mcause);
//     printf("c->mstatus: "); debug(c->mstatus);
//     printf("c->mepc: "); debug(c->mepc);
//     printf("c->pdir: "); debug((uint32_t)c->pdir);
// }

/*irq Interrupt Request 中断请求*/
Context* __am_irq_handle(Context *c) {
  if (user_handler) {
    Event ev = {0};
    // debugContext(c);
    switch (c->mcause) {
      case (uintptr_t)-1: ev.event = EVENT_YIELD;   break;
      case (uintptr_t) 0:
      case (uintptr_t) 1: 
      case (uintptr_t) 2: 
      case (uintptr_t) 3: 
      case (uintptr_t) 4:
      case (uintptr_t) 7:
      case (uintptr_t) 8:
      case (uintptr_t) 9: 
      case (uintptr_t) 13: 
      case (uintptr_t) 19: ev.event = EVENT_SYSCALL; break;
      default: assert(0); ev.event = EVENT_ERROR; break;
    }

    c = user_handler(ev, c);
    assert(c != NULL);
  }
  c->mepc = c->mepc + 4;
  return c;
}

extern void __am_asm_trap(void);

/*
 * 用于进行CTE相关的初始化操作. 其中它还接受一个来自操作系统的事件处理回调函数的指针, 
 * 当发生事件时, CTE将会把事件和相关的上下文作为参数, 来调用这个回调函数, 交由操作系统进行后续处理.
 * 
 * 对于riscv32来说, 直接将异常入口地址设置到mtvec寄存器中即可.
 * cte_init()函数做的第二件事是注册一个事件处理回调函数, 这个回调函数由yield test提供
 */
bool cte_init(Context*(*handler)(Event, Context*)) {
  // initialize exception entry
  asm volatile("csrw mtvec, %0" : : "r"(__am_asm_trap));

  // register event handler
  user_handler = handler;

  return true;
}

Context *kcontext(Area kstack, void (*entry)(void *), void *arg) {
  return NULL;
}

/*
 * riscv32提供ecall指令作为自陷指令, 并提供一个mtvec寄存器来存放异常入口地址.
 * 为了保存程序当前的状态, riscv32提供了一些特殊的系统寄存器, 叫控制状态寄存器(CSR寄存器). 
 * 在PA中, 我们只使用如下3个CSR寄存器:
 * mepc寄存器 - 存放触发异常的PC
 * mstatus寄存器 - 存放处理器的状态
 * mcause寄存器 - 存放触发异常的原因
 * 
 * 用于进行自陷操作, 会触发一个编号为EVENT_YIELD事件. 
 * 不同的ISA会使用不同的自陷指令来触发自陷操作, 具体实现请RTFSC.
 * 
 * riscv32通过mret指令从异常处理过程中返回, 它将根据mepc寄存器恢复PC.
 */
void yield() {
#ifdef __riscv_e
  asm volatile("li a5, -1; ecall");
#else
  asm volatile("li a7, -1; ecall");
#endif
}

bool ienabled() {
  return false;
}

void iset(bool enable) {
}
