#include <am.h>
#include <nemu.h>
#include <klib.h>
#include <stdio.h>

static AddrSpace kas = {};
static void* (*pgalloc_usr)(int) = NULL;
static void (*pgfree_usr)(void*) = NULL;
static int vme_enable = 0;

static Area segments[] = {      // Kernel memory mappings
  NEMU_PADDR_SPACE
};

#define USER_SPACE RANGE(0x40000000, 0x80000000)


/*
 * satp CSR寄存器共三个字段：
 * 31-31 为MODE字段：1 表示开启32位分页； 0 则是关闭
 * 30-22 为ASID字段：ASID（Address Space Identifier，地址空间标识）字段是可选的，可用于降低上下文切换的开销。
 * 21-0  为PPN字段：PPN 字段以4 KiB页为单位存放根页表的物理页号。
 * set_satp函数看来是将MODE字段设置为1，将ASID字段设置为全0，然后来自AddrSpace kas的pdir则是页目录的地址
 * 因为RISCV32中地址都是4KiB对齐，所以物理地址的后12位一定为0，我们直接将pdir右移12位存放进去
 */
static inline void set_satp(void *pdir) {
  Log("set_satp with address 0x%p", pdir);
  uintptr_t mode = 1ul << (__riscv_xlen - 1);
  asm volatile("csrw satp, %0" : : "r"(mode | ((uintptr_t)pdir >> 12)));
}

static inline uintptr_t get_satp() {
  uintptr_t satp;
  asm volatile("csrr %0, satp" : "=r"(satp));
  return satp << 12;
}

/*
 * 用于进行VME相关的初始化操作. 
 * 其中它还接受两个来自操作系统的页面分配回调函数的指针, 让AM在必要的时候通过这两个回调函数来申请/释放一页物理页.
 * 由于页表位于内存中, 但计算机启动的时候, 内存中并没有有效的数据, 因此我们不可能让计算机启动的时候就开启分页机制. 
 * 操作系统为了启动分页机制, 首先需要准备一些内核页表. 框架代码已经为我们实现好这一功能了
 *
 * 以riscv32为例, vme_init()将设置页面分配和回收的回调函数, 
 * 然后调用map()来填写内核虚拟地址空间(kas)的页目录和页表, 
 * 最后设置一个叫satp(Supervisor Address Translation and Protection)的CSR寄存器来开启分页机制. 
 * 这样以后, Nanos-lite就运行在分页机制之上了.
 */
bool vme_init(void* (*pgalloc_f)(int), void (*pgfree_f)(void*)) {
  pgalloc_usr = pgalloc_f;  // 对应于ics2023/nanos-lite/src/mm.c中的pg_alloc和pg_free
  pgfree_usr = pgfree_f;
 
  /*
   * kas为内核虚拟地址空间，AddrSpace 这个虚拟地址空间描述符包括pgsize页面大小，
   * area表示虚拟地址空间中用户态的范围
   * ptr是一个ISA相关的地址空间描述符指针, 用于指示具体的映射, 即物理地址
   */
  kas.ptr = pgalloc_f(PGSIZE);

  /*
   * 调用map()来填写内核虚拟地址空间(kas)的页目录和页表
   */
  int i;
  for (i = 0; i < LENGTH(segments); i ++) {
    void *va = segments[i].start;
    Log("vme_init: Kernel Virtual Space[0x%p, 0x%p) get %d Bit map with Paddr [0x%p, 0x%p)", va, (void *)segments[i].end, ((uintptr_t)segments[i].end - (uintptr_t)segments[i].start), va, (void *)segments[i].end);
    for (; va < segments[i].end; va += PGSIZE) {
      map(&kas, va, va, 0);
    }
  }

  // 最后设置一个叫satp(Supervisor Address Translation and Protection)的CSR寄存器来开启分页机制.
  Log("vme_init kernel page dir address: 0x%p", kas.ptr);
  set_satp(kas.ptr);
  Log("vme_init kernel page dir address: 0x%p", kas.ptr);
  vme_enable = 1;

  return true;
}

/*
 * 虚存机制, 说白了就是个映射(或函数). 也就是说, 本质上虚存管理要做的事情, 就是在维护这个映射. 
 * 但这个映射应该是每个进程都各自维护一份, 因此我们需要如下的两个API:
 */
// 我自己添加的为内核线程创建地址空间
// 我发现这玩意是不需要的
// void kprotect(AddrSpace *as) {
//   as->ptr = kas.ptr;
// }

// 创建一个默认的地址空间
void protect(AddrSpace *as) {
  // typedef uintptr_t PTE; in abstract-machine/am/src/platform/nemu/include/nemu.h
  PTE *updir = (PTE*)(pgalloc_usr(PGSIZE));
  as->ptr = updir;
  as->area = USER_SPACE;
  as->pgsize = PGSIZE;
  // map kernel space
  memcpy(updir, kas.ptr, PGSIZE);
}

// 销毁指定的地址空间
void unprotect(AddrSpace *as) {
}

void __am_get_cur_as(Context *c) {
  c->pdir = (vme_enable ? (void *)get_satp() : NULL);
  Log("__am_get_cur_as: Save now page dir address 0x%p in current context", c->pdir);
}

void __am_switch(Context *c) {
  if (vme_enable && c->pdir != NULL) {
    set_satp(c->pdir);
    Log("__am_switch: Context have change so to change page dir address 0x%p", c->pdir);
  }
  Log("__am_switch: vme_enable is false or new context page dir is NULL so not change and keep page dir address 0x%p", c->pdir);
}

/*
 * 有了地址空间, 我们还需要有相应的API来维护它们. 于是很自然就有了如下的API:
 * 它用于将地址空间as中虚拟地址va所在的虚拟页, 以prot的权限映射到pa所在的物理页. 
 * 当prot中的present位为0时, 表示让va的映射无效. 由于我们不打算实现保护机制, 因此权限prot暂不使用.
 *
 * map()是VME中的核心API, 它需要在虚拟地址空间as的页目录和页表中填写正确的内容, 
 * 使得将来在分页模式下访问一个虚拟页(参数va)时, 硬件进行page table walk后得到的物理页, 
 * 正是之前在调用map()时给出的目标物理页(参数pa). 这再次体现了分页是一个软硬协同才能工作的机制: 
 * 如果map()没有正确地填写这些内容, 将来硬件进行page table walk的时候就无法取得正确的物理页.
 *
 * 对于x86和riscv32, vme_init()会通过map()来填写内核虚拟地址空间的映射. 
 * 这些映射十分特殊, 它们的va和pa是相同的, 我们将它们称为"恒等映射"(identical mapping). 
 * 在硬件开启分页机制之后, CPU访问的物理地址就跟分页机制关闭时相同, 
 * 从而在无需修改其它代码的情况下, 达到"Nanos-lite看起来像是直接运行在物理内存上"的效果. 
 * 建立这样一个映射也有利于Nanos-lite进行内存管理: 
 * 即使在分页模式下, Nanos-lite可以把内存的物理地址直接当做虚拟地址来访问, 访问的结果正好是相应的物理地址.
 */
void map(AddrSpace *as, void *va, void *pa, int prot) {
  assert(as != NULL);
  assert((uintptr_t)va % PGSIZE == 0);
  assert((uintptr_t)pa % PGSIZE == 0);
  // 首先在页目录上查找，页目录的基地址为as->ptr, 本来是satp.PPN 也可以给出一级页表的基地址
  PTE *pg_dir = (PTE *)as->ptr;
  // 然后计算出虚拟地址va在页目录上的下标, 需要注意的是地址是直接以B为单位的
  assert(sizeof(PTE) == 4); // 目前我们的机器和操作系统都是32位，这里先断言下
  PTE mega = sizeof(PTE) * 1024 * 1024;
  int pd_idx = ((PTE) va) / ((PTE) mega);
  assert(pd_idx < 1024 && pd_idx >= 0); // 目前我们采用的是SV32 的分页方案，先断言下
  // 取出页目录的页表项(PTE)的内容, 页表项中的内容为4B = 32bit
  PTE pte = *(pg_dir + pd_idx);
  if (pte == 0) { // 说明虚拟地址空间[4MB * pd_idx, 4MB * (pd_idx + 1))还没有被映射(使用)
    pte = (PTE)pgalloc_usr(PGSIZE);  // 创建二级页表
    //pte的低12位必定为0，因为默认页表为4KiB，则所有页表的物理地址都应该对齐4KiB,即物理地址的低12位都是0
    assert((pte & ((((uintptr_t) 1) << 12) - 1)) == 0);
    *(pg_dir + pd_idx) = pte;
  }
  // 继续查看虚拟地址va在二级页表中的位置
  PTE *pg_tab = (PTE *)pte; // 二级页表基地址
  PTE kilo = sizeof(PTE) * 1024;
  int pt_idx = ((PTE) va) % ((PTE) mega) / ((PTE) kilo);
  assert(pt_idx < 1024 && pt_idx >= 0);
  // 将虚拟地址va对应的物理地址存放到二级页表的页表项上
  // 页表项的高31~10用于存放物理地址, pa的低12位必定为0
  assert((((PTE) pa) & ((((uintptr_t) 1) << 12) - 1)) == 0);
  // 因为页表项有22位用于存放物理地址，但是我们物理地址有效位只有32 - 12 = 20位
  *(pg_tab + pt_idx) = ((PTE) pa) >> 2;
  // 权限prot暂不使用
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
/*
 * 用于创建用户进程上下文. 我们之前已经介绍过这个API, 但加入虚存管理之后, 我们需要对这个API的实现进行一些改动, 具体改动会在下文介绍.
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

  //printf("start: 0x%x end:0x%x\n", (uintptr_t)low_sp, (uintptr_t)top_sp);

  Context *c = (Context *)low_sp;
  //c->gpr[0] = (uintptr_t)0; // $0 其实写不写无所谓，因为$0寄存器不参与保存和恢复上下文
  //c->gpr[2] = (uintptr_t)low_sp;  // sp 其实写不写无所谓，因为sp寄存器不参与保存和恢复上下文
  c->mstatus = (uintptr_t)0x1800;
  c->mepc = (uintptr_t)entry;
  c->pdir = as->ptr; // 将用户地址空间保存到上下文
  return c;
}
