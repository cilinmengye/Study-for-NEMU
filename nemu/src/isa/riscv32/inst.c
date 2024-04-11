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

#include "local-include/reg.h"
#include <cpu/cpu.h>
#include <cpu/ifetch.h>
#include <cpu/decode.h>

#define R(i) gpr(i)
#define Mr vaddr_read
#define Mw vaddr_write

enum {
  TYPE_I, TYPE_U, TYPE_S, TYPE_J,
  TYPE_N, // none
};

#define src1R() do { *src1 = R(rs1); } while (0)
#define src2R() do { *src2 = R(rs2); } while (0)
/*宏SEXT用于符号扩展*/
#define immI() do { *imm = SEXT(BITS(i, 31, 20), 12); } while(0)
#define immU() do { *imm = SEXT(BITS(i, 31, 12), 20) << 12; } while(0)
#define immS() do { *imm = (SEXT(BITS(i, 31, 25), 7) << 5) | BITS(i, 11, 7); } while(0)
#define immJ() do { *imm = SEXT((BITS(i, 31, 31) << 20) | (BITS(i, 19, 12) << 12) | \
                           (BITS(i, 20, 20) << 11) | (BITS(i, 30, 21) << 1), 21); } while(0)

/*
 * 刚才我们只知道了指令的具体操作(比如auipc是将当前PC值与立即数相加并写入寄存器), 但我们还是不知道操作对象(比如立即数是多少, 写入到哪个寄存器). 为了解决这个问题, 代码需要进行进一步的译码工作, 
 * 这是通过调用decode_operand()函数来完成的. 这个函数将会根据传入的指令类型type来进行操作数的译码, 
 * 译码结果将记录到函数参数rd, src1, src2和imm中, 它们分别代表目的操作数的寄存器号码, 两个源操作数和立即数.
 */
static void decode_operand(Decode *s, int *rd, word_t *src1, word_t *src2, word_t *imm, int type) {
  uint32_t i = s->isa.inst.val;
  /*宏BITS用于位抽取*/
  int rs1 = BITS(i, 19, 15);
  int rs2 = BITS(i, 24, 20);
  /*decode_operand会首先统一对目标操作数进行寄存器操作数的译码, 即调用*rd = BITS(i, 11, 7), 不同的指令类型可以视情况使用rd*/
  *rd     = BITS(i, 11, 7);
  switch (type) {
    case TYPE_I: src1R();          immI(); break;
    case TYPE_U:                   immU(); break;
    case TYPE_S: src1R(); src2R(); immS(); break;
    case TYPE_J:                   immJ(); break;
  }
}

static int decode_exec(Decode *s) {
  int rd = 0;
  word_t src1 = 0, src2 = 0, imm = 0;
  s->dnpc = s->snpc;

#define INSTPAT_INST(s) ((s)->isa.inst.val)
/*
 * such as INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc  , U, R(rd) = s->pc + imm);
 * after decode_operand we know instruct and opeator number, and then   __VA_ARGS__ ; \
 * is execute R(rd) = s->pc + imm);
 */
#define INSTPAT_MATCH(s, name, type, ... /* execute body */ ) { \
  decode_operand(s, &rd, &src1, &src2, &imm, concat(TYPE_, type)); \
  __VA_ARGS__ ; \
}
  /* 
   * INSTPAT_START();和INSTPAT_END();
   * #define INSTPAT_START(name) { const void ** __instpat_end = &&concat(__instpat_end_, name);
   * #define INSTPAT_END(name)   concat(__instpat_end_, name): ; }
   * 是用来当有指令匹配成功后，并且完成指令执行操作后，直接goto到INSTPAT_END()用的，即跳掉其他的INSTPAT
   */
  INSTPAT_START();
  /*INSTPAT(模式字符串, 指令名称, 指令类型, 指令执行操作);
   * imm is immediate number
   */
  INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc  , U, R(rd) = s->pc + imm);
  /*
   * Mr read the number len of 8 byte from pmem which address is src+imm 
   * ord_t vaddr_read(vaddr_t addr, int len) 
   */
  INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu    , I, R(rd) = Mr(src1 + imm, 1));
  /*
   * Mw write the number len of 8 byte data--src2 to pmem which address is src1+imm 
   * void vaddr_write(vaddr_t addr, int len, word_t data)
   */
  INSTPAT("??????? ????? ????? 000 ????? 01000 11", sb     , S, Mw(src1 + imm, 1, src2));
  /*#define NEMUTRAP(thispc, code) set_nemu_state(NEMU_END, thispc, code)*/
  // void set_nemu_state(int state, vaddr_t pc, int halt_ret) {
  //   difftest_skip_ref();
  //   nemu_state.state = state;
  //   nemu_state.halt_pc = pc;
  //   nemu_state.halt_ret = halt_ret;
  // }
  INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak , N, NEMUTRAP(s->pc, R(10))); // R(10) is $a0
  /*
   * mv rd, rs1 伪指令,实际被扩展为 addi rd, rs1, 0
   * li rd, immediate 伪指令扩展形式为 addi rd, x0, imm.
   */
  INSTPAT("??????? ????? ????? 000 ????? 00100 11", addi   , I, R(rd) = src1 + imm); //将rs1寄存器的内容src1加上立即数保存到寄存器rd中
  /*
   * 将下一条指令的地址保存,到ra寄存器中，并加上偏移量imm继续执行指令
   * 通过jal指令，程序可以实现函数调用和返回的功能。在函数调用时，jal指令将函数的入口地址存储到目标寄存器中，并跳转到函数的起始地址执行；
   * 在函数返回时，通过目标寄存器中保存的返回地址，程序可以回到函数调用的位置继续执行。
   * s->dnpc += imm - 4: 因为我们在inst_fetch函数的调用过程中已经对snpc+=4了,即我们提前更新了PC
   * 
   * snpc是下一条静态指令, 而dnpc是下一条动态指令. 对于顺序执行的指令, 它们的snpc和dnpc是一样的; 
   * 但对于跳转指令, snpc和dnpc就会有所不同, dnpc应该指向跳转目标的指令
   * 
   * j offset 跳转 (Jump). 伪指令(Pseudoinstruction ，等同于 jal x0, offset.
   */
  INSTPAT("??????? ????? ????? ??? ????? 11011 11", jal    , J, R(rd) = s->snpc; s->dnpc += imm - 4); 
  /*
   * jalr rd, offset(rs1) t =pc+4; pc=(x[rs1]+sext(offset))&~1; x[rd]=t
   * 
   * ret pc = x[1] 返回(Return). 伪指令(Pseudoinstruction)  实际被扩展为jalr x0, 0(x1)
   */
  INSTPAT("??????? ????? ????? 010 ????? 11001 11", jalr   , I, s->dnpc = (src1 + imm) & (~1); R(rd) = s->snpc);
  
  INSTPAT("??????? ????? ????? 010 ????? 01000 11", sw     , S, Mw(src1 + imm, 4, src2));
  /*在模式匹配过程的最后有一条inv的规则, 表示"若前面所有的模式匹配规则都无法成功匹配, 则将该指令视为非法指令*/
  INSTPAT("??????? ????? ????? ??? ????? ????? ??", inv    , N, INV(s->pc));
  INSTPAT_END();

  R(0) = 0; // reset $zero to 0

  return 0;
}

int isa_exec_once(Decode *s) {
  s->isa.inst.val = inst_fetch(&s->snpc, 4);
  return decode_exec(s);
}
