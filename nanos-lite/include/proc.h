#ifndef __PROC_H__
#define __PROC_H__

#include <common.h>
#include <memory.h>

#define STACK_SIZE (8 * PGSIZE)


/*
 * 我们首先需要在加载用户进程之前为其创建地址空间. 由于地址空间是进程相关的, 我们将AddrSpace结构体作为PCB的一部分. 
 * 这样以后, 我们只需要在context_uload()的开头调用protect(), 就可以实现地址空间的创建. 
 */
typedef union {
  uint8_t stack[STACK_SIZE] PG_ALIGN;
  struct {
    Context *cp;
    AddrSpace as; // in ics2023/abstract-machine/am/include/am.h
    // we do not free memory, so use `max_brk' to determine when to call _map()
    uintptr_t max_brk;
  };
} PCB;

extern PCB *current;

#endif
