#ifndef ARCH_H__
#define ARCH_H__

#ifdef __riscv_e
#define NR_REGS 16
#else
#define NR_REGS 32
#endif

/*
 * 除了通用寄存器之外, 上下文还包括:
 * 触发异常时的PC和处理器状态: mepc mstatus
 * 异常号: mcause
 * 地址空间: pdir
 */
struct Context {
  // TODO: fix the order of these members to match trap.S
  void *pdir;
  uintptr_t gpr[NR_REGS], mepc, mstatus, mcause;
};

#ifdef __riscv_e
#define GPR1 gpr[15] // a5
#else
#define GPR1 gpr[17] // a7
#endif

#define GPR2 gpr[0]
#define GPR3 gpr[0]
#define GPR4 gpr[0]
#define GPRx gpr[0]

#endif
