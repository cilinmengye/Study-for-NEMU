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
  TYPE_R, TYPE_I, TYPE_S, TYPE_U, TYPE_J, TYPE_B, TYPE_I_shamt, 
  TYPE_N, // none
};
void iringbuf_get(Decode s);

#define src1R() do { *src1 = R(rs1); } while (0)
#define src2R() do { *src2 = R(rs2); } while (0)
/*宏SEXT用于符号扩展*/
#define immI() do { *imm = SEXT(BITS(i, 31, 20), 12); } while(0)
#define immU() do { *imm = SEXT(BITS(i, 31, 12), 20) << 12; } while(0)
#define immS() do { *imm = (SEXT(BITS(i, 31, 25), 7) << 5) | BITS(i, 11, 7); } while(0)
#define immJ() do { *imm = SEXT((BITS(i, 31, 31) << 20) | (BITS(i, 19, 12) << 12) | \
                           (BITS(i, 20, 20) << 11) | (BITS(i, 30, 21) << 1), 21); } while(0)
#define immB() do { *imm = SEXT((BITS(i, 31, 31) << 12) | (BITS(i, 7,   7) << 11) | \
                           (BITS(i, 30, 25) <<  5) | (BITS(i, 11,  8) << 1), 13); } while(0)
#define immI_shamt() do { *imm = BITS(i, 25, 20); } while (0)

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
    case TYPE_R:         src1R(); src2R(); break;
    case TYPE_I: src1R();          immI(); break;
    case TYPE_U:                   immU(); break;
    case TYPE_S: src1R(); src2R(); immS(); break;
    case TYPE_J:                   immJ(); break;
    case TYPE_B: src1R(); src2R(); immB(); break;
    case TYPE_I_shamt:    src1R(); immI_shamt(); break;
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
  /*
   * lui rd, immediate x[rd] = sext(immediate[31:12] << 12) 将符号位扩展的 20 位立即数 immediate 左移 12 位，并将低 12 位置零，写入 x[rd]中
   */
  INSTPAT("??????? ????? ????? ??? ????? 01101 11", lui    , U, R(rd) = imm);
  /*
   * auipc rd, immediate x[rd] = pc + sext(immediate[31:12] << 12)
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
   * sb rs2, offset(rs1) M[x[rs1] + sext(offset)] = x[rs2][7:0]
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
   * lb rd, offset(rs1) x[rd] = sext(M[x[rs1] + sext(offset)][7:0])
   */
  INSTPAT("??????? ????? ????? 000 ????? 00000 11", lb     , I, R(rd) = SEXT(Mr(src1 + imm, 1), 8));
  /*
   * lh rd, offset(rs1) x[rd] = sext(M[x[rs1] + sext(offset)][15:0])
   */
  INSTPAT("??????? ????? ????? 001 ????? 00000 11", lh     , I, R(rd) = SEXT(Mr(src1 + imm, 2), 16));
  /*
   * lw rd, offset(rs1) x[rd] = sext(M[x[rs1] + sext(offset)][31:0])
   * 从地址 x[rs1] + sign-extend(offset)读取四个字节，写入 x[rd]
   */
  INSTPAT("??????? ????? ????? 010 ????? 00000 11", lw     , I, R(rd) = SEXT(Mr(src1 + imm, 4), 32));
  /*
   * lbu rd, offset(rs1) x[rd] = M[x[rs1] + sext(offset)][7:0]
   * 无符号字节加载 (Load Byte, Unsigned). I-type, RV32I and RV64I.
   */
  INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu    , I, R(rd) = Mr(src1 + imm, 1));
  /*
   * lhu rd, offset(rs1) x[rd] = M[x[rs1] + sext(offset)][15:0] 无符号半字加载
   */
  INSTPAT("??????? ????? ????? 101 ????? 00000 11", lhu    , I, R(rd) = Mr(src1 + imm, 2));
  /*
   * mv rd, rs1 伪指令,实际被扩展为 addi rd, rs1, 0
   * li rd, immediate 伪指令扩展形式为 addi rd, x0, imm.
   * addi rd, rs1, immediate x[rd] = x[rs1] + sext(immediate)
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
  INSTPAT("??????? ????? ????? 000 ????? 11001 11", jalr   , I, R(rd) = s->snpc; s->dnpc = (src1 + imm) & ~1);
  /*
   * sw rs2, offset(rs1) M[x[rs1] + sext(offset) = x[rs2][31: 0] 存字
   */
  INSTPAT("??????? ????? ????? 010 ????? 01000 11", sw     , S, Mw(src1 + imm, 4, src2));
  /* 
   * sh rs2, offset(rs1) M[x[rs1] + sext(offset) = x[rs2][15: 0]
   */
  INSTPAT("??????? ????? ????? 001 ????? 01000 11", sh     , S, Mw(src1 + imm, 2, src2));
  /*
   * seqz rd, rs1  x[rd] = (x[rs1] == 0) 等于 0 则置位(Set if Equal to Zero). 伪指令(Pseudoinstruction),实际被扩展为 sltiu rd, rs1,
   * sltiu rd, rs1, immediate  x[rd] = (x[rs1] <𝑢 sext(immediate))
   * 比较 x[rs1]和有符号扩展的 immediate，比较时视为无符号数。如果 x[rs1]更小，向 x[rd]写入1，否则写入 0。
   */
  INSTPAT("??????? ????? ????? 011 ????? 00100 11", sltiu  , I, R(rd) = (src1 < imm));
  /*
   * srli rd, rs1, shamt x[rd] = (x[rs1] ≫𝑢 shamt) 立即数逻辑右移
   * 把寄存器x[rs1]右移shamt位，空出的位置填入0，结果写入x[rd]。对于RV32I，仅当shamt[5]=0时，指令才是有效的。
   */
  INSTPAT("000000 ?????? ????? 101 ????? 00100 11", srli   , I_shamt, if (BITS(s->isa.inst.val, 25, 25) == 0) R(rd) = (src1 >> imm));
  /* 
   * srai rd, rs1, shamt x[rd] = (x[rs1] ≫𝑠 shamt)
   * 立即数算术右移(Shift Right Arithmetic Immediate)
   * 把寄存器 x[rs1]右移 shamt 位，空位用 x[rs1]的最高位填充，结果写入 x[rd]。
   * 对于 RV32I，仅当 shamt[5]=0 时指令有效。
   */
  INSTPAT("010000 ?????? ????? 101 ????? 00100 11", srai   , I_shamt, if (BITS(s->isa.inst.val, 25, 25) == 0) R(rd) = (((int32_t)src1) >> imm));
  /*
   * slli rd, rs1, shamt x[rd] = x[rs1] ≪ shamt 立即数逻辑左移
   * 把寄存器x[rs1]左移shamt位，空出的位置填入0，结果写入x[rd]。对于RV32I，仅当shamt[5]=0时，指令才是有效的。
   */
  INSTPAT("000000 ?????? ????? 001 ????? 00100 11", slli   , I_shamt, if (BITS(s->isa.inst.val, 25, 25) == 0) R(rd) = (src1 << imm));
  /*
   * andi rd, rs1, immediate x[rd] = x[rs1] & sext(immediate)
   */
  INSTPAT("??????? ????? ????? 111 ????? 00100 11", andi   , I, R(rd) = (src1 & imm));
  /*
   * xori rd, rs1, immediate x[rd] = x[rs1] ^ sext(immediate)
   */
  INSTPAT("??????? ????? ????? 100 ????? 00100 11", xori   , I, R(rd) = (src1 ^ imm));
  /*
   * ori rd, rs1, immediate x[rd] = x[rs1] | sext(immediate)
   * 或立即数。I 型，在 RV32I 和 RV64I 中。将 x[rs1] 和符号扩展后的 immediate 按位或的结果写入 x[rd]。
   */
  INSTPAT("??????? ????? ????? 110 ????? 00100 11", ori    , I, R(rd) = (src1 | imm));
  /*
   * slt rd, rs1, rs2 x[rd] = (x[rs1] <𝑠 x[rs2]) 小于则置位(Set if Less Than)
   */ 
  INSTPAT("0000000 ????? ????? 010 ????? 01100 11", slt    , R, R(rd) = ((int32_t)src1 < (int32_t)src2) ); 
  /*
   * sltu rd, rs1, rs2 x[rd] = (x[rs1] <𝑢 x[rs2])
   */
  INSTPAT("0000000 ????? ????? 011 ????? 01100 11", sltu   , R, R(rd) = (src1 < src2) ); 
  /*
   * sll rd, rs1, rs2 x[rd] = x[rs1] ≪ x[rs2] 逻辑左移(Shift Left Logical).
   */
  INSTPAT("0000000 ????? ????? 001 ????? 01100 11", sll    , R, R(rd) = (src1 << src2) ); 
  /*
   * srl rd, rs1, rs2 x[rd] = (x[rs1] ≫𝑢 x[rs2]) 逻辑右移
   */
  INSTPAT("0000000 ????? ????? 101 ????? 01100 11", srl    , R, R(rd) = (src1 >> src2) ); 
    /*
   * sra rd, rs1, rs2 x[rd] = (x[rs1] ≫𝑠 x[rs2]) 算术右移
   */
  INSTPAT("0100000 ????? ????? 101 ????? 01100 11", sra    , R, R(rd) = ((int32_t)src1 >> src2) ); 
  /*
   * xor rd, rs1, rs2 x[rd] = x[rs1] ^ x[rs2]
   */
  INSTPAT("0000000 ????? ????? 100 ????? 01100 11", xor    , R, R(rd) = (src1 ^ src2) ); 
  /*
   * or rd, rs1, rs2 x[rd] = x[rs1] | 𝑥[𝑟𝑠2] 把寄存器 x[rs1]和寄存器 x[rs2]按位取或，结果写入 x[rd]
   */
  INSTPAT("0000000 ????? ????? 110 ????? 01100 11", or     , R, R(rd) = (src1 | src2) ); 
  /*
   * sub rd, rs1, rs2 x[rd] = x[rs1] − x[rs2]
   */
  INSTPAT("0100000 ????? ????? 000 ????? 01100 11", sub    , R, R(rd) = (src1 - src2) ); 
  /*
   * add rd, rs1, rs2 x[rd] = x[rs1] + x[rs2]
   */
  INSTPAT("0000000 ????? ????? 000 ????? 01100 11", add    , R, R(rd) = (src1 + src2) ); 
  /*
   * and rd, rs1, rs2 x[rd] = x[rs1] & x[rs2]
   */
  INSTPAT("0000000 ????? ????? 111 ????? 01100 11", and    , R, R(rd) = (src1 & src2) ); 
  /*
   * mul rd, rs1, rs2 x[rd] = x[rs1] × x[rs2]
   */
  INSTPAT("0000001 ????? ????? 000 ????? 01100 11", mul    , R, R(rd) = (src1 * src2) );
  /*
   * mulh rd, rs1, rs2 x[rd] = (x[rs1] s×s x[rs2]) >>s XLEN 
   * XLEN = 32
   * 将 x[rs2] 与 x[rs1] 视为补码并相乘，乘积的高位写入 x[rd]。
   */
  INSTPAT("0000001 ????? ????? 001 ????? 01100 11", mulh   , R,
          int32_t a = src1;
          int32_t b = src2;
          int64_t c = (int64_t)a * b;
          R(rd) = c >> 32);
  /* 
   * mulhu rd, rs1, rs2 x[rd] = (x[rs1] u×u x[rs2]) >>u XLEN
   * 高位无符号乘。R 型，在 RV32M 和 RV64M 中。
   * 将 x[rs2] 与 x[rs1] 视为无符号数并相乘，乘积的高位写入 x[rd]
   */
  INSTPAT("0000001 ????? ????? 011 ????? 01100 11", mulhu  , R,
          uint64_t c = (uint64_t)src1 * src2;
          R(rd) = c >> 32);
  /* 
   * div rd, rs1, rs2 x[rd] = x[rs1] ÷s x[rs2]
   * 将这些数视为二进制补码
   */
  INSTPAT("0000001 ????? ????? 100 ????? 01100 11", div    , R, R(rd) = ((int32_t)src1 / (int32_t)src2) );
  /*
   * divu rd, rs1, rs2 x[rd] = x[rs1] ÷u x[rs2]
   * 无符号除。R 型，在 RV32M 和 RV64M 中。
   * x[rs1] 除以 x[rs2]（无符号除法），结果向零舍入，将商写入 x[rd]
   */
  INSTPAT("0000001 ????? ????? 101 ????? 01100 11", divu   , R, R(rd) = (src1 / src2) );
  /*
   * rem rd, rs1, rs2 x[rd] = x[rs1] %𝑠 x[rs2]
   * x[rs1]除以 x[rs2]，向 0 舍入，都视为 2 的补码，余数写入 x[rd]。
   */
  INSTPAT("0000001 ????? ????? 110 ????? 01100 11", rem    , R, R(rd) = ((int32_t)src1 % (int32_t)src2) );
  /*
   * remu rd, rs1, rs2 x[rd] = x[rs1] %u x[rs2]
   * x[rs1]除以 x[rs2]，向 0 舍入，都视为 2 的补码，余数写入 x[rd]。
   * 将 x[rs1] 和 x[rs2] 视为无符号数并相除，向 0 舍入，将余数写入 x[rd]。
   */
  INSTPAT("0000001 ????? ????? 111 ????? 01100 11", remu   , R, R(rd) = (src1 % src2) );
  /*
   * beqz rs1, offset if (rs1 == 0) pc += sext(offset)  伪指令 可视为 beq rs1, x0, offset.
   * beq rs1, rs2, offset if (rs1 == rs2) pc += sext(offset) 相等时分支
   */
  INSTPAT("??????? ????? ????? 000 ????? 11000 11", beq    , B, s->dnpc += src1 == src2 ? imm - 4 : 0);
  /*
   * bne rs1, rs2, offset if (rs1 ≠ rs2) pc += sext(offset) 不相等时分支
   */
  INSTPAT("??????? ????? ????? 001 ????? 11000 11", bne    , B, s->dnpc += src1 != src2 ? imm - 4 : 0);
  /*
   * blez rs2, offset if (rs2 ≤s 0) pc += sext(offset) 伪指令 可视为 bge x0, rs2, offset.
   * 
   * bge rs1, rs2, offset if (rs1 ≥s rs2) pc += sext(offset) 大于等于时分支
   * 若寄存器 x[rs1]的值大于等于寄存器 x[rs2]的值（均视为二进制补码）把 pc 的值设为当前值加上符号位扩展的偏移 offset。
   */
  INSTPAT("??????? ????? ????? 101 ????? 11000 11", bge    , B, s->dnpc += (int32_t)src1 >= (int32_t)src2 ? imm - 4 : 0);
  /*
   * bgeu rs1, rs2, offset if (rs1 ≥u rs2) pc += sext(offset) 无符号大于等于时分支
   * 若寄存器 x[rs1]的值大于等于寄存器 x[rs2]的值（均视为无符号数）
   */
  INSTPAT("??????? ????? ????? 111 ????? 11000 11", bgeu   , B, s->dnpc +=  src1 >= src2 ? imm - 4 : 0);
  /*
   * blt rs1, rs2, offset if (rs1 <s rs2) pc += sext(offset) 小于时分支
   * 若寄存器 x[rs1]的值小于寄存器 x[rs2]的值（均视为二进制补码）
   */
  INSTPAT("??????? ????? ????? 100 ????? 11000 11", bit    , B, s->dnpc += (int32_t)src1 < (int32_t)src2 ? imm - 4 : 0);
  /*
   * bltu rs1, rs2, offset if (rs1 <u rs2) pc += sext(offset) 无符号小于时分支
   */
  INSTPAT("??????? ????? ????? 110 ????? 11000 11", bltu   , B, s->dnpc += src1 < src2 ? imm - 4 : 0);
  /*在模式匹配过程的最后有一条inv的规则, 表示"若前面所有的模式匹配规则都无法成功匹配, 则将该指令视为非法指令*/
  INSTPAT("??????? ????? ????? ??? ????? ????? ??", inv    , N, INV(s->pc));
  INSTPAT_END();

  R(0) = 0; // reset $zero to 0

  return 0;
}

int isa_exec_once(Decode *s) {
  s->isa.inst.val = inst_fetch(&s->snpc, 4);
  IFDEF(CONFIG_IRINGTRACE, iringbuf_get(*s));
  return decode_exec(s);
}
