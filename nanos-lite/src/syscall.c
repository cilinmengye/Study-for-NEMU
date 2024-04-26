#include <common.h>
#include <unistd.h>
#include "syscall.h"

int fs_open(const char *pathname, int flags, int mode);
size_t fs_read(int fd, void *buf, size_t len);
size_t fs_write(int fd, const void *buf, size_t len);
size_t fs_lseek(int fd, size_t offset, int whence);
int fs_close(int fd);

static void sys_exit(Context *c){
  halt(c->GPRx);
}

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

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;
  printf("do_syscall: %d\n", (int)a[0]);
  switch (a[0]) {
    case (uintptr_t) 0: sys_exit(c);  break;
    case (uintptr_t) 1: sys_yield(c); break;
    case (uintptr_t) 2: sys_open(c);  break;
    case (uintptr_t) 3: sys_read(c);  break;
    case (uintptr_t) 4: sys_write(c); break;
    case (uintptr_t) 7: sys_close(c); break;
    case (uintptr_t) 8: sys_lseek(c); break;
    case (uintptr_t) 9: sys_brk(c);   break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
}
