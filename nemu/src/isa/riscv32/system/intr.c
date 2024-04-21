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

/*
 * 为了支撑自陷操作, 同时测试异常入口地址是否已经设置正确, 
 * 你需要在NEMU中实现isa_raise_intr()函数来模拟上文提到的异常响应机制.
 * 
 * 你需要在自陷指令的实现中调用isa_raise_intr(), 而不要把异常响应机制的代码放在自陷指令的helper函数中实现,
 * 因为在后面我们会再次用到isa_raise_intr()函数.
 * 将当前PC值保存到mepc寄存器
 * 在mcause寄存器中设置异常号
 * 从mtvec寄存器中取出异常入口地址
 * 跳转到异常入口地址
 */
word_t isa_raise_intr(word_t NO, vaddr_t epc) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * Then return the address of the interrupt/exception vector.
   */
  cpu.csrs.mcause = NO;
  cpu.csrs.mepc = epc;
  return cpu.csrs.mtvec;
}

word_t isa_query_intr() {
  return INTR_EMPTY;
}
