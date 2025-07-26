#include <common.h>
#include <unistd.h>
#include <sys/time.h>
#include <proc.h>
#include <stdio.h>
#include "syscall.h"
#include <stdio.h>

int fs_open(const char *pathname, int flags, int mode);
size_t fs_read(int fd, void *buf, size_t len);
size_t fs_write(int fd, const void *buf, size_t len);
size_t fs_lseek(int fd, size_t offset, int whence);
int fs_close(int fd);
void naive_uload(PCB *pcb, const char *filename);
 void context_uload(PCB *pcb, const char *filename, char *const *argv, char *const *envp);
extern PCB *current;
void switch_boot_pcb();

static void sys_yield(Context *c){
  yield();
  c->GPRx = 0;
}

/*
 * man 2 open
 * return the new file descriptor (a nonnegative integer), or -1 if an error occurred 
 */
static void sys_open(Context *c){
  char *pathname = (char *)c->GPR2;
  int flags = c->GPR3;
  int mode = c->GPR4; 
  c->GPRx = fs_open(pathname, flags, mode);
}

static void sys_read(Context *c){
  int fd = c->GPR2;
  void *buf = (void *)c->GPR3;
  size_t len = c->GPR4;
  c->GPRx = fs_read(fd, buf, len);
}

static void sys_write(Context *c){
  int fd = c->GPR2;
  void *buf = (void *)c->GPR3;
  size_t len = c->GPR4;
  c->GPRx = fs_write(fd, buf, len);
}

/*
 * man 2 close
 * close() returns zero on success.  On error, -1 is returned, and errno is set appropriately.
 */
static void sys_close(Context *c){
  int fd = c->GPR2;
  c->GPRx = fs_close(fd);
}

static void sys_lseek(Context *c){
  int fd = c->GPR2;
  size_t offset = c->GPR3;
  int whence = c->GPR4;
  c->GPRx = fs_lseek(fd, offset, whence);
}

/*
 * 我们还需要在Nanos-lite中实现sys_brk的功能. 
 * 由于目前Nanos-lite还是一个单任务操作系统, 空闲的内存都可以让用户程序自由使用,
 * 因此我们只需要让sys_brk系统调用总是返回0即可, else return -1
 * 表示堆区大小的调整总是成功. 在PA4中, 我们会对这一系统调用进行修改, 实现真正的内存分配.
 */
static void sys_brk(Context *c){
  c->GPRx = 0;
}

/*
 * 它的作用是结束当前程序的运行, 并启动一个指定的程序
 * 如果它执行成功, 就不会返回到当前程序中
 * 为了实现这个系统调用, 你只需要在相应的系统调用处理函数中调用naive_uload()就可以了. 
 * 目前我们只需要关心filename即可, argv和envp这两个参数可以暂时忽略.
 *
 * PA4: 因为用户进程的参数还是应该由用户来指定的. 
 * 于是最好能有一个方法能把用户指定的参数告诉操作系统, 让操作系统来把指定的参数放到新进程的用户栈里面. 
 * 为了实现带参数的SYS_execve, 我们可以在sys_execve()中直接调用context_uload()
 * * 我们假设用户进程A将要通过SYS_execve来执行另一个新程序B.
 * * * 如何在A的执行流中创建用户进程B?
 * * * 如何结束A的执行流?
 * 
 * 在加载B时, Nanos-lite使用的是A的用户栈! 这意味着在A的执行流结束之前, A的用户栈是不能被破坏的. 
 * 因此heap.end附近的用户栈是不能被B复用的, 我们应该申请一段新的内存作为B的用户栈, 来让Nanos-lite把B的参数放置到这个新分配的用户栈里面.
 * 我们可以让context_uload()统一通过调用new_page()函数来获得用户栈的内存空间.
 *
 * int execve(const char *filename, char *const argv[], char *const envp[]);
 */
static void sys_execve(Context *c){
  const char *fname = (const char *)c->GPR2;
  char *const* argv = (char *const *)c->GPR3;
  char *const* envp = (char *const *)c->GPR4;

  // printf("sys_execve fname: %s\n", fname);
  // //debug
  // for (int i = 0; argv[i]; i++) printf("argv[%d]: 0x%x %s\n", i, (uintptr_t)argv[i], argv[i]);
  // if (argv[0] == NULL) printf("argv[0]: NULL\n");
  // for (int i = 0; envp[i]; i++) printf("envp[%d]: 0x%x %s\n", i, (uintptr_t)envp[i], envp[i]);
  // if (envp[0] == NULL) printf("envp[0]: NULL\n");

  // if (current->cp == c) printf("yes\n");

  //printf("nanos-lite sys_execve fname: %s\n", fname);
  //naive_uload(NULL, fname);
  //printf("1\n");
  context_uload(current, fname, argv, envp);
  //printf("2\n");
  switch_boot_pcb();
  //printf("3\n");
  yield();
  c->GPRx = 0;
}

/* 
 * 关于输入设备, 我们先来看看时钟. 时钟比较特殊, 大部分操作系统并没有把它抽象成一个文件, 
 * 而是直接提供一些和时钟相关的系统调用来给用户程序访问. 
 * 在Nanos-lite中, 我们也提供一个SYS_gettimeofday系统调用, 用户程序可以通过它读出当前的系统时间.
 * 
 * gettimeofday() and settimeofday() return 0 for success, or -1 for fail‐ure (in which case errno is set appropriately).
 */
static void sys_gettimeofday(Context *c){
  struct timeval *tv = (struct timeval *)c->GPR2;
  /*这里不支持实现tz,调用时传入参数NULL*/
  //assert(c->GPR3 == NULL);
  assert(tv != NULL);
  uint64_t us = io_read(AM_TIMER_UPTIME).us;
  tv->tv_sec = us / 1000000;
  tv->tv_usec = us % 1000000;
  c->GPRx = 0;
}

static void sys_exit(Context *c){
  const char *front = "/bin/nterm";
  char *const argv[] = {NULL};
  char *const envp[] = {NULL};
  c->GPR2 = (uintptr_t)front;
  c->GPR3 = (uintptr_t)argv;
  c->GPR4 = (uintptr_t)envp;
  sys_execve(c);
  //halt(c->GPRx);
}

#define STRACE 1

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;
  //printf("do_syscall: %d\n", (int)a[0]);

  #ifdef STRACE
  switch (a[0]) {
    case (uintptr_t) 0: Log("sys_exit");   break;
    case (uintptr_t) 1: Log("sys_yield");  break;
    case (uintptr_t) 2: break; Log("sys_open");   break;
    case (uintptr_t) 3: break; Log("sys_read");   break;
    case (uintptr_t) 4: break; Log("sys_write");  break;
    case (uintptr_t) 7: break; Log("sys_close");  break;
    case (uintptr_t) 8: break; Log("sys_lseek");  break;
    case (uintptr_t) 9: Log("sys_brk");    break;
    case (uintptr_t) 13: Log("sys_execve");          break;
    case (uintptr_t) 19: Log("sys_gettimeofday(c)"); break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
  #endif
  
  switch (a[0]) {
    case (uintptr_t) 0: sys_exit(c);  break;
    case (uintptr_t) 1: sys_yield(c); break;
    case (uintptr_t) 2: sys_open(c);  break;
    case (uintptr_t) 3: sys_read(c);  break;
    case (uintptr_t) 4: sys_write(c); break;
    case (uintptr_t) 7: sys_close(c); break;
    case (uintptr_t) 8: sys_lseek(c); break;
    case (uintptr_t) 9: sys_brk(c);   break;
    case (uintptr_t) 13: sys_execve(c);       break;
    case (uintptr_t) 19: sys_gettimeofday(c); break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
}
