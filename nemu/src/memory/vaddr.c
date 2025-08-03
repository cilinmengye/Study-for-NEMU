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
#include <memory/paddr.h>

/*
 * 为了使用这些API, 你需要对NEMU中虚拟地址访问的函数进行一些修改. 
 * 具体地, 首先需要通过isa_mmu_check()来根据当前的系统状态判断一次虚拟地址的访问应该如何进行:
 * 如果isa_mmu_check()返回MMU_DIRECT, 表示可以直接把该地址作为物理地址来访问, 此时直接调用paddr_read()或paddr_write()即可
 * 如果isa_mmu_check()返回MMU_TRANSLATE, 表示该访问需要通过MMU进行地址转换, 
 * 此时需要先调用isa_mmu_translate()进行地址转换, 然后再通过地址转换后的物理地址来调用paddr_read()或paddr_write()
 * 根据API的定义, isa_mmu_check()还可以返回MMU_FAIL, 表示访问失败, 需要抛出异常, 不过这种情况在PA中不会出现
 */

word_t vaddr_ifetch(vaddr_t addr, int len) {
  if (isa_mmu_check(addr, len, MEM_TYPE_IFETCH) == MMU_DIRECT) return paddr_read(addr, len);
  paddr_t paddr = isa_mmu_translate(addr, len, MEM_TYPE_IFETCH);

  if (paddr != MEM_RET_FAIL && paddr != MEM_RET_CROSS_PAGE) {
    paddr = paddr | ((addr << 20) >> 20); //  ((addr << 20) >> 20) 取 addr 后 12 位
    Assert(paddr == addr, "now vaddr == paddr");
    return paddr_read(paddr, len);
  }
  Assert(0, "meet MEM_RET_FAIL or MEM_RET_CROSS_PAGE");
}

word_t vaddr_read(vaddr_t addr, int len) {
  if (isa_mmu_check(addr, len, MEM_TYPE_READ) == MMU_DIRECT) return paddr_read(addr, len);
  paddr_t paddr = isa_mmu_translate(addr, len, MEM_TYPE_READ);

  if (paddr != MEM_RET_FAIL && paddr != MEM_RET_CROSS_PAGE) {
    paddr = paddr | ((addr << 20) >> 20); //  ((addr << 20) >> 20) 取 addr 后 12 位
    Assert(paddr == addr, "now vaddr == paddr");
    return paddr_read(paddr, len);
  }
  Assert(0, "meet MEM_RET_FAIL or MEM_RET_CROSS_PAGE");
}

void vaddr_write(vaddr_t addr, int len, word_t data) {
  if (isa_mmu_check(addr, len, MEM_TYPE_WRITE) == MMU_DIRECT) return paddr_write(addr, len, data);
  paddr_t paddr = isa_mmu_translate(addr, len, MEM_TYPE_WRITE);

  if (paddr != MEM_RET_FAIL && paddr != MEM_RET_CROSS_PAGE) {
    paddr = paddr | ((addr << 20) >> 20); //  ((addr << 20) >> 20) 取 addr 后 12 位
    Assert(paddr == addr, "now vaddr == paddr");
    return paddr_write(addr, len, data);
  }
  Assert(0, "meet MEM_RET_FAIL or MEM_RET_CROSS_PAGE");
}
