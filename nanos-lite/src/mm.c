#include <memory.h>

static void *pf = NULL;

/*
 * 我们应该申请一段新的内存作为B的用户栈, 来让Nanos-lite把B的参数放置到这个新分配的用户栈里面.
 * 为了实现这一点, 我们可以让context_uload()统一通过调用new_page()函数来获得用户栈的内存空间.
 * new_page()函数在nanos-lite/src/mm.c中定义, 它会通过一个pf指针来管理堆区, 
 * 用于分配一段大小为nr_page * 4KB的连续内存区域, 并返回这段区域的首地址. 
 * 我们让context_uload()通过new_page()来分配32KB的内存作为用户栈, 这对PA中的用户程序来说已经足够使用了.
 * 此外为了简化, 我们在PA中无需实现free_page().
 * 
 * 测试程序navy-apps/tests/exec-test, 它会以参数递增的方式不断地执行自身. 
 * 不过由于我们没有实现堆区内存的回收, exec-test在运行一段时间之后, 
 * new_page()就会把0x3000000/0x83000000附近的内存分配出去, 导致用户进程的代码段被覆盖. 
 * 目前我们无法修复这一问题, 你只需要看到exec-test可以正确运行一段时间即可.
 */
void* new_page(size_t nr_page) {
  assert(pf);
  uint8_t *oldpf = (uint8_t *)pf;
  pf = (void *)(oldpf + nr_page * 4 * 1024);
  return (void *)oldpf;
}

#ifdef HAS_VME
/*
 * pg_alloc()的参数是分配空间的字节
 * 但我们保证AM通过回调函数调用pg_alloc()时申请的空间总是页面大小的整数倍, 
 * 因此可以通过调用new_page()来实现pg_alloc(). 此外pg_alloc()还需要对分配的页面清零.
 */
static void* pg_alloc(int n) {
  size_t nr_page = (n - 1 + 4 * 1024) / (4 * 1024); // nr_page向上取整
  void *ptr = new_page(nr_page);
  memset(ptr, 0, n);
  return ptr;
}
#endif

void free_page(void *p) {
  panic("not implement yet");
}

/* The brk() system call handler. */
int mm_brk(uintptr_t brk) {
  return 0;
}

/*
 * 只需要在nanos-lite/include/common.h中定义宏HAS_VME, 
 * Nanos-lite在初始化的时候首先就会调用init_mm()函数(在nanos-lite/src/mm.c中定义)来初始化MM. 
 * 这里的MM是指存储管理器(Memory Manager)模块, 它专门负责分页相关的存储管理.
 */
/*
 * 目前初始化MM的工作有两项, 第一项工作是将TRM提供的堆区起始地址作为空闲物理页的首地址, 
 * 这样以后, 将来就可以通过new_page()函数来分配空闲的物理页了. 
 * 为了简化实现, MM可以采用顺序的方式对物理页进行分配, 而且分配后无需回收. 
 * 第二项工作是调用AM的vme_init()函数. 
 */
void init_mm() {
  pf = (void *)ROUNDUP(heap.start, PGSIZE);
  Log("free physical pages starting from %p", pf);

#ifdef HAS_VME
  vme_init(pg_alloc, free_page);
#endif
}
