#include <proc.h>

#define MAX_NR_PROC 4

void naive_uload(PCB *pcb, const char *filename);
void context_uload(PCB *pcb, const char *filename, char *const argv[], char *const envp[]);

static PCB pcb[MAX_NR_PROC] __attribute__((used)) = {};
static PCB pcb_boot = {};
PCB *current = NULL;

void switch_boot_pcb() {
  current = &pcb_boot;
}


#ifdef STRACE

/*
 * 判断当前PCB是谁
 * 返回值：
 * -1 表示为 pcb_boot
 * i  表示为 pcb[i]
 */
int classify_PCB() {
  if (current == &pcb_boot) return -1;
  for (int i = 0; i < MAX_NR_PROC; i++)
    if (current == &pcb[i])
      return i;
  Assert(0, "Undefine PCB %p", current);
}

#endif


void hello_fun(void *arg) {
  int j = 1;
  while (1) {
    Log("Hello World from Nanos-lite with arg '%s' for the %dth time!", (uintptr_t)arg, j);
    j ++;
    yield();
  }
}

// Nanos-lite的context_kload()函数(框架代码未给出该函数的原型) 
// 它进一步封装了创建内核上下文的过程: 调用kcontext()来创建上下文, 并把返回的指针记录到PCB的cp中
// Context *kcontext(Area kstack, void (*entry)(void *), void *arg) 
void context_kload(PCB* pcb, void (*entry)(void *), void *arg) {
  pcb->cp = kcontext((Area) { pcb->stack, pcb->stack + sizeof(PCB) }, entry, arg);
}

void init_proc() {

  //context_uload(&pcb[0], "/bin/hello");
  // context_kload(&pcb[0], hello_fun, "test1");
  // char *const argv[] = {"/bin/pal", "--skip", NULL};
  // char *const envp[] = {NULL};
  // context_uload(&pcb[1], "/bin/pal", argv, envp);
  //context_kload(&pcb[1], hello_fun, "test2");
  Log("Initializing processes...");

  switch_boot_pcb();

  //context_kload(&pcb[0], hello_fun, "context_kload text");
  // char *const argv[] = {"/bin/nterm", NULL};
  // char *const envp[] = {NULL};
  // context_uload(&pcb[1], "/bin/nterm", argv, envp);
  // char *const argv[] = {"/bin/pal", NULL};
  // char *const envp[] = {NULL};
  // context_uload(&pcb[1], "/bin/pal", argv, envp);

  // char *const argv[] = {"/bin/dummy", NULL};
  // char *const envp[] = {NULL};
  // context_uload(&pcb[1], "/bin/dummy", argv, envp);

  char *const argv[] = {"/bin/bmp-test", "a", "b", NULL};
  char *const envp[] = {"HOME=/home/cilinmengye", "USER=cilinmengye", NULL};
  context_uload(&pcb[1], "/bin/bmp-test", argv, envp);

  // // load program here
  // naive_uload(NULL, "/bin/bmp-test");
}

Context* schedule(Context *prev) {
  // current->cp = prev;
  // current = (current == &pcb[0] ? &pcb[1] : &pcb[0]);
  // return current->cp;
  current = &pcb[1];
  return current->cp;
}
