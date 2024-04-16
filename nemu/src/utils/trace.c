#include <common.h>
#include <cpu/decode.h>

#ifdef CONFIG_ITRACE
#define IRINGBUF_SIZE 16

static Decode iringbuf[IRINGBUF_SIZE];
/*The next instruction should be placed at the index in iringbuf*/
static int iringbuf_nextIdx = 0;

void iringbuf_get(Decode s){
  iringbuf[iringbuf_nextIdx++] = s;
  if (iringbuf_nextIdx >= IRINGBUF_SIZE)
    iringbuf_nextIdx = 0;
}

static void iringbuf_translate(Decode *s){
  char *p = s->logbuf;
  p += snprintf(p, sizeof(s->logbuf), FMT_WORD ":", s->pc);
  int ilen = s->snpc - s->pc;
  int i;
  uint8_t *inst = (uint8_t *)&s->isa.inst.val;
  for (i = ilen - 1; i >= 0; i --) {
    p += snprintf(p, 4, " %02x", inst[i]);
  }
  int ilen_max = MUXDEF(CONFIG_ISA_x86, 8, 4);
  int space_len = ilen_max - ilen;
  if (space_len < 0) space_len = 0;
  space_len = space_len * 3 + 1;
  memset(p, ' ', space_len);
  p += space_len;

#ifndef CONFIG_ISA_loongarch32r
  void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);
  disassemble(p, s->logbuf + sizeof(s->logbuf) - p,
      MUXDEF(CONFIG_ISA_x86, s->snpc, s->pc), (uint8_t *)&s->isa.inst.val, ilen);
#else
  p[0] = '\0'; // the upstream llvm does not support loongarch32r
#endif
}

/*
 * 一般来说, 我们只会关心出错现场前的trace, 在运行一些大程序的时候, 运行前期的trace大多时候没有查看甚至输出的必要. 
 * 一个很自然的想法就是, 我们能不能在客户程序出错(例如访问物理内存越界)的时候输出最近执行的若干条指令呢?
 * 要实现这个功能其实并不困难, 我们只需要维护一个很简单的数据结构 - 环形缓冲区(ring buffer)即可
*/
void iringbuf_display(){
  int iringbuf_nowIdx = (iringbuf_nextIdx - 1) < 0 ? 31 : iringbuf_nextIdx - 1; 
  int i;


  for (i = 0; i < IRINGBUF_SIZE; i++){
    if (i == iringbuf_nowIdx)
      printf("%-4s","-->");
    else 
      printf("%-4s","   ");
    iringbuf_translate(&iringbuf[i]);
    printf("%s\n",iringbuf[i].logbuf);
  }
}
#endif

void mtraceRead_display(paddr_t addr, int len){
  printf("read address = " FMT_PADDR " at pc = " FMT_WORD " with byte = %d\n",
      addr, cpu.pc, len);
}

void mtraceWrite_display(paddr_t addr, int len, word_t data){
  printf("write address = " FMT_PADDR " at pc = " FMT_WORD " with byte = %d and data =" FMT_WORD "\n",
      addr, cpu.pc, len, data);
}

void init_ftrace(const char *elf_file){
  
}
