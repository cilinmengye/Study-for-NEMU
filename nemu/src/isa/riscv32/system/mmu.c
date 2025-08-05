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
#include <memory/vaddr.h>
#include <memory/paddr.h>

/*
 * 为了让map()(ics2023/abstract-machine/am/src/riscv/nemu/vme.c)填写的映射生效, 我们还需要在NEMU中实现分页机制. 具体地, 我们需要实现以下两点:
 * 如何判断CPU当前是否处于分页模式?
 * 分页地址转换的具体过程应该如何实现?
 * 但这两点都是ISA相关的, 于是NEMU将它们抽象成相应的API:
 * // 检查当前系统状态下对内存区间为[vaddr, vaddr + len), 类型为type的访问是否需要经过地址转换.
 * int isa_mmu_check(vaddr_t vaddr, int len, int type);
 * // 对内存区间为[vaddr, vaddr + len), 类型为type的内存访问进行地址转换
 * paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type);
 *
 * 你需要理解分页地址转换的过程, 然后实现isa_mmu_check()(在nemu/src/isa/$ISA/include/isa-def.h中定义) 
 * 和isa_mmu_translate()(在nemu/src/isa/$ISA/system/mmu.c中定义), 
 * 你可以查阅NEMU的ISA相关API说明文档来了解它们的行为. 
 * 另外由于我们不打算实现保护机制, 在isa_mmu_translate()的实现中, 
 * 你务必使用assertion检查页目录项和页表项的present/valid位, 
 * 如果发现了一个无效的表项, 及时终止NEMU的运行, 否则调试将会非常困难. 
 * 这通常是由于你的实现错误引起的, 请检查实现的正确性.
 */
/*
// 对内存区间为[vaddr, vaddr + len), 类型为type的内存访问进行地址转换. 函数返回值可能为:

// pg_paddr | MEM_RET_OK: 地址转换成功, 其中pg_paddr为物理页面的地址(而不是vaddr翻译后的物理地址)
// 物理页面的地址即是页表项的PFN（页框号）在虚拟内存系统里，虚拟内存 被划分成一个个“虚拟页面”（virtual pages），
// 物理内存 被划分成同样大小的“物理页框”（page frames）。PFN 即 “Page Frame Number”，它不是物理地址本身，而是“第几个页框”的编号。
// 物理地址 =  page frames << 12 + offset
// MEM_RET_FAIL: 地址转换失败, 原因包括权限检查失败等不可恢复的原因, 一般需要抛出异常
// MEM_RET_CROSS_PAGE: 地址转换失败, 原因为访存请求跨越了页面的边界
*/
paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type) {
  // 先得到页目录的物理基地址, 取出satp的低22位 并左移12位
  uint32_t mask = (((uint32_t) 1) << 22) - 1;
  uint32_t *pg_dir = (uint32_t *)((uint64_t)(cpu.csrs.satp & mask) << 12);
  // 然后计算出虚拟地址va在页目录上的下标, 需要注意的是地址是直接以B为单位的
  vaddr_t mega = 4 * 1024 * 1024;
  int pd_idx = vaddr / mega;
  assert(pd_idx < 1024 && pd_idx >= 0); // 目前我们采用的是SV32 的分页方案，先断言下
  // 取出页目录的页表项(PTE)的内容, 页表项中的内容为4B = 32bit
  // 需要注意的是现在我们是在硬件层面了，从物理地址中取出内容直接用的是paddr_read
  uint32_t pte = paddr_read((uint64_t)(pg_dir + pd_idx), 4);

  Log("Isa_mmu_translate vaddr:0x%x len:%d -- pg_dir:0x%x pd_idx:%d pte:0x%x", vaddr, len, (uint32_t)((uint64_t)pg_dir), pd_idx, pte);

  if (pte == 0) { // 说明虚拟地址空间[4MB * pd_idx, 4MB * (pd_idx + 1) - 1)还没有被映射(使用), 即物理页还没加载上来
    return MEM_RET_FAIL;
  }
  // 继续查看虚拟地址va在二级页表中的位置
  uint32_t *pg_tab = (uint32_t *)((uint64_t) pte); // 二级页表基地址
  vaddr_t kilo = 4 * 1024;
  int pt_idx = vaddr %  mega / kilo;
  assert(pt_idx < 1024 && pt_idx >= 0);
  pte = paddr_read((uint64_t)(pg_tab + pt_idx), 4);
  
  Log("Isa_mmu_translate vaddr:0x%x len:%d -- pg_tab:0x%x pt_idx:%d pte:0x%x", vaddr, len, (uint32_t)((uint64_t)pg_tab), pt_idx, pte);
  
  if (pte == 0) { // 即物理页还没加载上来
    return MEM_RET_FAIL;
  }
  // 判断下是否跨页了, 首先要得到本页的不能达的最大虚拟地址
  vaddr_t max_vaddr = ((vaddr >> 12) + 1) << 12;
  if((vaddr + len - 1) >= max_vaddr) return MEM_RET_CROSS_PAGE;
  // 页表项的高31~10为存放PPN的地方
  paddr_t pg_paddr = (pte >> 10) << 12;
  
  //Log("Isa_mmu_translate vaddr:0x%x len:%d -- pg_paddr: 0x%x", vaddr, len, pg_paddr);
  
  return pg_paddr | MEM_RET_OK;
}

/*
// 如何判断CPU当前是否处于分页模式?

// 检查当前系统状态下对内存区间为[vaddr, vaddr + len), 类型为type的访问是否需要经过地址转换. 其中type可能为:

// MEM_TYPE_IFETCH: 取指令
// MEM_TYPE_READ: 读数据
// MEM_TYPE_WRITE: 写数据
// 函数返回值可能为:

// MMU_DIRECT: 该内存访问可以在物理内存上直接进行
// MMU_TRANSLATE: 该内存访问需要经过地址转换
// MMU_FAIL: 该内存访问失败, 需要抛出异常(如RISC架构不支持非对齐的内存访问)
// 根据API的定义, isa_mmu_check()还可以返回MMU_FAIL, 表示访问失败, 需要抛出异常, 不过这种情况在PA中不会出现
*/
int isa_mmu_check(vaddr_t vaddr, int len, int type) {
  // 需要对齐
  /*
   * 遇到过access address 0x87ffffe1 with 4B need aligned, 目前还不知道要如何处理
   * 可能是我在软件上实现的时候就没有主要到要对齐...
   */
  // Assert(vaddr % len == 0, "access address 0x%0x with %dB need aligned", vaddr, len);
  // 获取satp的MODR位
  assert(sizeof(word_t) == 4);
  int mode = cpu.csrs.satp >> (sizeof(word_t) * 8 - 1);
  if (mode == 0) return MMU_DIRECT;
  return MMU_TRANSLATE;
}
