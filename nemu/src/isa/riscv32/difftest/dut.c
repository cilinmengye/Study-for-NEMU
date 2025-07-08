/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <cpu/difftest.h>
#include "../local-include/reg.h"
#define NR_GPR MUXDEF(CONFIG_RVE, 16, 32)

extern const char *regs[];

/*
 * 你需要实现isa_difftest_checkregs()函数, 把通用寄存器和PC与从DUT中读出的寄存器的值进行比较. 
 * 若对比结果一致, 函数返回true; 如果发现值不一样 函数返回false
 * 特别地, isa_difftest_checkregs()对比结果不一致时, 
 * 第二个参数pc应指向导致对比结果不一致的指令, 可用于打印提示信息.
 * 上文在介绍API约定的时候, 提到了寄存器状态r需要把寄存器按照某种顺序排列. 
 * 你首先需要RTFSC, 从中找出这一顺序, 并检查你的NEMU实现是否已经满足约束.
 */
bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc) {
  // cpu.pc 表示的是当前指令执行完后的下一条指令地址
  // 参数pc表示的当前指令地址
  if (ref_r->pc != cpu.pc) {
    Log("DiffTest CheckRegs in PC: %s", ANSI_FMT("ref_r->pc != cpu.pc, 即下一条要执行的指令地址不同", ANSI_FG_RED));
    Log("Ref = " FMT_WORD, ref_r->pc);
    Log("Local = " FMT_WORD, cpu.pc);
    return false;
  }
  for (int i = 0; i < NR_GPR; i++) {
    if (ref_r->gpr[i] != cpu.gpr[i]) {
      Log("DiffTest CheckRegs in Regs: \33[1;41m %s \33[0m", regs[i]);
      Log("Ref = " FMT_WORD, ref_r->gpr[i]);
      Log("Local = " FMT_WORD, cpu.gpr[i]);
      return false;
    }
  }
  return true;
}

void isa_difftest_attach() {
}
