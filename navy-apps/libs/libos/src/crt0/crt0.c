#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

int main(int argc, char *argv[], char *envp[]);
extern char **environ;
void call_main(uintptr_t *args) {
  char *empty[] =  {NULL };
  environ = empty;

  uintptr_t sp_val = 0;
  asm volatile ("mv %0, sp" : "=r"(sp_val));
  printf("sp = 0x%lx\n", (unsigned long)sp_val);
  
  exit(main(0, empty, empty));
  assert(0);
}
