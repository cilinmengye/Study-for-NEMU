#include <common.h>
#include <utils.h>
#include <cpu/decode.h>
#include <device/map.h>
#include <fcntl.h>
#include <libelf.h>
#include <gelf.h>

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


#ifdef CONFIG_MTRACE
void mtraceRead_display(paddr_t addr, int len){
  printf("mtrace: read address = " FMT_PADDR " at pc = " FMT_WORD " with byte = %d\n",
      addr, cpu.pc, len);
}

void mtraceWrite_display(paddr_t addr, int len, word_t data){
  printf("mtrace: write address = " FMT_PADDR " at pc = " FMT_WORD " with byte = %d and data =" FMT_WORD "\n",
      addr, cpu.pc, len, data);
}
#endif


#ifdef CONFIG_FTRACE

Elf* elf = NULL;
int formBlank = 1;
char elfbuf[512];

char* find_symbol_by_addr(Elf* elf, vaddr_t addr) {
  char* ret = NULL;
  Elf_Scn *scn = NULL;        // 当前遍历到的节 (Section)
  Elf_Data *sym_data = NULL;  // 指向符号表数据的指针
  Elf_Data *str_data = NULL;  // 指向符号名字符串表数据的指针
  GElf_Shdr sym_shdr;         // 用于保存当前符号表节头
  size_t sym_count = 0;       // 符号表中总的符号数量

  // 遍历所有节，寻找符号表节
  while ((scn = elf_nextscn(elf, scn)) != NULL) {
    // 读取当前节的节头信息到 sym_shdr
    if (gelf_getshdr(scn, &sym_shdr) == NULL) Assert(0, "gelf_getshdr failed");
    // 判断是否为符号表节
    if (sym_shdr.sh_type == SHT_SYMTAB) {
      // 获取符号表本身的数据指针
      sym_data = elf_getdata(scn, NULL);
      Assert(sym_data != NULL, "Get symbol table fail");
      // 获取字符串表，关联的字符串表位于节索引 sym_shdr.sh_link
      Elf_Scn *str_scn = elf_getscn(elf, sym_shdr.sh_link);
      Assert(str_scn != NULL, "Failed to get string table section");
      str_data = elf_getdata(str_scn, NULL);
      break;
    }
  }
  if (sym_data == NULL) Assert(0, "No symbol table found in ELF file");
  if (str_data == NULL) Assert(0, "No string table found in ELF file");
  // 计算符号条目数量 = 节大小 / 每条目大小
  sym_count = sym_shdr.sh_size / sym_shdr.sh_entsize;
  // 因为有可能这个地址并非是函数符号地址，所以需要记录下
  // 在符号表条目中查找地址最接近且不大于 addr 的函数符号
  for (size_t i = 0; i < sym_count; i++) {
    GElf_Sym sym;
    gelf_getsym(sym_data, i, &sym);
    // 只关注函数类型的符号
    if (GELF_ST_TYPE(sym.st_info) != STT_FUNC) continue;
    vaddr_t start = (vaddr_t)sym.st_value;
    vaddr_t end   = (vaddr_t)(start + sym.st_size);
    if (addr >= start && addr < end) {
      // 获取其在字符串表中的地址
      ret = (char*)str_data->d_buf + sym.st_name;
      return ret;
    }
  }
  return ret;
}

void ftraceInst_get(char* type, vaddr_t instAddr, vaddr_t toAddr) {
  if (strcmp(type, "ret") == 0) formBlank--;
  char *sym_name = find_symbol_by_addr(elf, toAddr);
  if (sym_name == NULL) sym_name = "???";
  // 然后将内容输出到log_file中
  char* p = elfbuf;
  // 先输出指令地址
  p += snprintf(p, sizeof(elfbuf), FMT_WORD ":", instAddr);
  // 再输出层次空格
  for (int i = 0; i < formBlank; i++) p += snprintf(p, sizeof(elfbuf), " ");
  // 再输出主体内容
  p += snprintf(p, sizeof(elfbuf), "%5s[%s@0x%08x]\n", type, sym_name, toAddr);
  Assert((p - elfbuf ) <= 512, "Ftrace elfbuf overflow");
  // 然后将p中的内容输出到log_file中
  log_write("%s", elfbuf);
  if (strcmp(type, "call") == 0) formBlank++;
}

/*
 * 需要初始化一下elf文件
 */
void init_ftrace(const char *elf_file) {
  Assert(elf_file != NULL, "ELF file can't be NULL");
  Assert(elf_version(EV_CURRENT) != EV_NONE, "ELF library initialization failed");
  int elf_fp = open(elf_file, O_RDONLY);
  Assert(elf_fp >= 0, "Failed to open ELF file");
  Log("ELF file path is %s", elf_file);
  elf = elf_begin(elf_fp, ELF_C_READ, NULL);
  Assert(elf != NULL, "elf_begin failed");

  snprintf(elfbuf, sizeof(elfbuf), "[Starting Ftrace]\n");
  log_write("%s", elfbuf);
}
#endif

void dtraceRead_display(void *addr, int len, IOMap *map){
  printf("dtrace: Drive Name = %s : read address = %p at pc = "FMT_WORD" with byte = %d\n",
        map->name, addr, cpu.pc, len);
}

void dtraceWrite_display(void *addr, int len, word_t data, IOMap *map){
  printf("dtrace: Drive Name = %s : write address = %p at pc = "FMT_WORD" with byte = %d and data = "FMT_WORD" \n",
        map->name, addr, cpu.pc, len, data);
}

