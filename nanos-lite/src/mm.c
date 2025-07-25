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
static void* pg_alloc(int n) {
  return NULL;
}
#endif

void free_page(void *p) {
  panic("not implement yet");
}

/* The brk() system call handler. */
int mm_brk(uintptr_t brk) {
  return 0;
}

void init_mm() {
  pf = (void *)ROUNDUP(heap.start, PGSIZE);
  Log("free physical pages starting from %p", pf);

#ifdef HAS_VME
  vme_init(pg_alloc, free_page);
#endif
}
