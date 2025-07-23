#include <am.h>
#include <nemu.h>
#include <klib.h>

static AddrSpace kas = {};
static void* (*pgalloc_usr)(int) = NULL;
static void (*pgfree_usr)(void*) = NULL;
static int vme_enable = 0;

static Area segments[] = {      // Kernel memory mappings
  NEMU_PADDR_SPACE
};

#define USER_SPACE RANGE(0x40000000, 0x80000000)

static inline void set_satp(void *pdir) {
  uintptr_t mode = 1ul << (__riscv_xlen - 1);
  asm volatile("csrw satp, %0" : : "r"(mode | ((uintptr_t)pdir >> 12)));
}

static inline uintptr_t get_satp() {
  uintptr_t satp;
  asm volatile("csrr %0, satp" : "=r"(satp));
  return satp << 12;
}

bool vme_init(void* (*pgalloc_f)(int), void (*pgfree_f)(void*)) {
  pgalloc_usr = pgalloc_f;
  pgfree_usr = pgfree_f;

  kas.ptr = pgalloc_f(PGSIZE);

  int i;
  for (i = 0; i < LENGTH(segments); i ++) {
    void *va = segments[i].start;
    for (; va < segments[i].end; va += PGSIZE) {
      map(&kas, va, va, 0);
    }
  }

  set_satp(kas.ptr);
  vme_enable = 1;

  return true;
}

void protect(AddrSpace *as) {
  PTE *updir = (PTE*)(pgalloc_usr(PGSIZE));
  as->ptr = updir;
  as->area = USER_SPACE;
  as->pgsize = PGSIZE;
  // map kernel space
  memcpy(updir, kas.ptr, PGSIZE);
}

void unprotect(AddrSpace *as) {
}

void __am_get_cur_as(Context *c) {
  c->pdir = (vme_enable ? (void *)get_satp() : NULL);
}

void __am_switch(Context *c) {
  if (vme_enable && c->pdir != NULL) {
    set_satp(c->pdir);
  }
}

void map(AddrSpace *as, void *va, void *pa, int prot) {
}

/*
 * 和内核线程不同, 用户进程的代码, 数据和堆栈都应该位于用户区, 而且需要保证用户进程能且只能访问自己的代码, 数据和堆栈. 
 * 为了区别开来, 我们把PCB中的栈称为内核栈, 位于用户区的栈称为用户栈. 
 * 于是我们需要一个有别于kcontext()的方式来创建用户进程的上下文
 *
 * 其中, 参数as用于限制用户进程可以访问的内存, 我们在下一阶段才会使用, 目前可以忽略它; 
 * kstack是内核栈, 用于分配上下文结构, entry则是用户进程的入口.
 * 由于目前我们忽略了as参数, 所以ucontext()的实现和kcontext()几乎一样, 甚至比kcontext()更简单: 
 * 连参数都不需要传递. 不过你还是需要思考, 对于用户进程来说, 它需要一个什么样的状态来开始执行呢?
 * 事实上, 用户栈的分配是ISA无关的, 所以用户栈相关的部分就交给Nanos-lite来进行
 */
Context *ucontext(AddrSpace *as, Area kstack, void *entry) {
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
  c->gpr[0] = (uintptr_t)0; // $0 其实写不写无所谓，因为$0寄存器不参与保存和恢复上下文
  c->gpr[2] = (uintptr_t)low_sp;  // sp 其实写不写无所谓，因为sp寄存器不参与保存和恢复上下文
  c->mstatus = (uintptr_t)0x1800;
  c->mepc = (uintptr_t)entry;
  return c;
}
