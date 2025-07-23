#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

int main(int argc, char *argv[], char *envp[]);
extern char **environ;

void call_main(uintptr_t *args) {

  uintptr_t sp_val = 0, a0_val = 0;
  asm volatile ("mv %0, sp" : "=r"(sp_val));
  asm volatile ("mv %0, a0" : "=r"(a0_val));
  printf("sp = 0x%lx a0 = 0x%lx args = 0x%lx\n", (unsigned long)sp_val, (unsigned long)a0_val, (unsigned long)args);

  // 按道理来说传参a0 == args == argc的地址
  uintptr_t p = (uintptr_t)args;
  int argc = *(int *)p;
  printf("argc: %d\n", argc);
  char **argv = (char **)(p + 4);
  printf("argv: %s %s\n", argv[0], argv[1]);
  char **envp = (char **)(p + 4 + (argc + 1) * sizeof(uintptr_t));

  //char *empty[] =  {NULL };
  environ = envp;

  // uintptr_t sp_val = 0;
  // asm volatile ("mv %0, sp" : "=r"(sp_val));
  // printf("sp = 0x%lx\n", (unsigned long)sp_val);
  
  exit(main(argc, argv, envp));
  assert(0);
}
