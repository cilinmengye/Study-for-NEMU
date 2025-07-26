#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>
#define Log(format, ...) \
  printf("\33[1;35m[%s,%d,%s] " format "\33[0m\n", \
      __FILE__, __LINE__, __func__, ## __VA_ARGS__)

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
      case (uintptr_t)(11): {
        #ifdef __riscv_e
          if (c->gpr[15] == (uintptr_t)(-1)) { // a5
            ev.event = EVENT_YIELD; 
            break;
          }
        #else
          if (c->gpr[17] == (uintptr_t)(-1)) { // a7
            ev.event = EVENT_YIELD;
            break;
          }
        #endif
        ev.event = EVENT_SYSCALL; break;
      }
      default: assert(0); ev.event = EVENT_NULL; break;
    }

    c = user_handler(ev, c);
    assert(c != NULL);
  }
  // 对于mips32的syscall和riscv32的ecall, 保存的是自陷指令的PC
  // 因此软件需要在适当的地方对保存的PC加上4, 使得将来返回到自陷指令的下一条指令.
  c->mepc = c->mepc + 4;

  //Log("Will jump to entry = %p", (void *)c->mepc);
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

/*
 * 创建内核线程的上下文是通过CTE提供的kcontext()函数
 * 其中kstack是栈的范围, entry是内核线程的入口, arg则是内核线程的参数. 
 * 此外, kcontext()要求内核线程不能从entry返回, 否则其行为是未定义的. 
 * 你需要在kstack的底部创建一个以entry为入口的上下文结构(目前你可以先忽略arg参数), 然后返回这一结构的指针.
 */
Context *kcontext(Area kstack, void (*entry)(void *), void *arg) {
  // Area kstack是要创建的线程的栈，我需要在这个栈中找好位置上下文结构内容
  int nr_regs = 0, xlen = 0, context_size = 0;

#ifndef __riscv_e
  nr_regs = 32;
#else
  nr_regs = 16;
#endif

#if __riscv_xlen == 32
  xlen = 4;
#else
  xlen = 8;
#endif
  
  context_size = (nr_regs + 3) * xlen;
  uint8_t *top_sp = (uint8_t *)kstack.end;  // 拿到栈的顶部指针, 注意这里栈顶指针初始是不能用的
  uint8_t *low_sp = top_sp - context_size;
  Context *c = (Context *)low_sp;
  //c->gpr[0] = (uintptr_t)0; // $0 其实写不写无所谓，因为$0寄存器不参与保存和恢复上下文
  //c->gpr[2] = (uintptr_t)low_sp;  // sp 其实写不写无所谓，因为sp寄存器不参与保存和恢复上下文
  c->gpr[10] = (uintptr_t)arg;  // a0
  c->mstatus = (uintptr_t)0x1800;
  c->mepc = (uintptr_t)entry;
  return c;
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
