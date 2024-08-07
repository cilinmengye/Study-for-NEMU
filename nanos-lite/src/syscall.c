#include <common.h>
#include <unistd.h>
#include <sys/time.h>
#include <proc.h>
#include "syscall.h"

int fs_open(const char *pathname, int flags, int mode);
size_t fs_read(int fd, void *buf, size_t len);
size_t fs_write(int fd, const void *buf, size_t len);
size_t fs_lseek(int fd, size_t offset, int whence);
int fs_close(int fd);
void naive_uload(PCB *pcb, const char *filename);

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
 */
static void sys_execve(Context *c){
  char *fname = (char *)c->GPR2;
  naive_uload(NULL, fname);
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
  c->GPR2 = (intptr_t)front;
  sys_execve(c);
  //halt(c->GPRx);
}

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;
  //printf("do_syscall: %d\n", (int)a[0]);
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
