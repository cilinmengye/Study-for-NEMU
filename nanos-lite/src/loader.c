#include <proc.h>
#include <elf.h>
#include <stdio.h>

#ifdef __LP64__
# define Elf_Ehdr Elf64_Ehdr
# define Elf_Phdr Elf64_Phdr
#else
# define Elf_Ehdr Elf32_Ehdr
# define Elf_Phdr Elf32_Phdr
#endif

#if defined(__ISA_AM_NATIVE__)
# define EXPECT_TYPE EM_X86_64
#elif defined(__riscv)
# define EXPECT_TYPE EM_RISCV
#else 
# error Unsupported ISA
#endif

size_t ramdisk_read(void *buf, size_t offset, size_t len);
size_t ramdisk_write(const void *buf, size_t offset, size_t len);
int fs_open(const char *pathname, int flags, int mode);
size_t fs_read(int fd, void *buf, size_t len);
size_t fs_lseek(int fd, size_t offset, int whence);
int fs_close(int fd);

static uintptr_t loader(PCB *pcb, const char *filename) {
  //TODO();
  //printf("nanos-lite loader filename: %s\n", filename);
  int fd = fs_open(filename, 0, 0);

  Elf_Ehdr elf_header;
  size_t getSize = fs_read(fd, &elf_header, sizeof(elf_header));
  //printf("get elf_header base offset: %d \n", 400143 + 0);
  //size_t getSize = ramdisk_read(&elf_header, 0, sizeof(elf_header));
  Assert(getSize <= sizeof(elf_header), "loader %s getSize: %d and elf_header: %d",
         filename, getSize, sizeof(elf_header));
  assert(*(uint32_t *)elf_header.e_ident == 0x464c457f);
  assert(elf_header.e_machine == EXPECT_TYPE);
  
  Elf_Phdr program_header;
  for (uint16_t i = 0; i < elf_header.e_phnum; i++){
    fs_lseek(fd, elf_header.e_phoff + i * sizeof(program_header), 0);
    getSize = fs_read(fd, &program_header, sizeof(program_header));
    //printf("get program_header base offset: %d\n", 400143 + elf_header.e_phoff + i * sizeof(program_header));
    //getSize = ramdisk_read(&program_header, elf_header.e_phoff + i * sizeof(program_header), sizeof(program_header));
    assert(getSize <= sizeof(program_header));
    if(program_header.p_type != PT_LOAD)
      continue;
    
    fs_lseek(fd, program_header.p_offset, 0);
    fs_read(fd, (void *)program_header.p_vaddr, program_header.p_filesz);
    //printf("set memory base offset: %d\n", 400143 + program_header.p_offset);
    //ramdisk_read((void *)program_header.p_vaddr, program_header.p_offset, program_header.p_filesz);

    if (program_header.p_memsz > program_header.p_filesz)
      memset((void *)(program_header.p_vaddr + program_header.p_filesz), 0, program_header.p_memsz - program_header.p_filesz);
  }
  fs_close(fd);
  return elf_header.e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  //printf("nanos-lite naive_uload filename: %s\n", filename);
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", (void *)entry);
  ((void(*)())entry) ();
}

/*
 * 哎呀, 栈指针寄存器可是ISA相关的, 在Nanos-lite里面不方便处理. 
 * 别着急, 还记得用户进程的那个_start吗? 在那里可以进行一些ISA相关的操作. 
 * 于是Nanos-lite和Navy作了一项约定: Nanos-lite把栈顶位置设置到GPRx中, 
 * 然后由Navy里面的_start来把栈顶位置真正设置到栈指针寄存器中.
 * Nanos-lite可以把上述工作封装到context_uload()函数中, 这样我们就可以加载用户进程了. 
 * context_uload(&pcb[1], "/bin/pal");
 *
 * 不过为了给用户进程传递参数, 你还需要修改context_uload()的原型:
 * void context_uload(PCB *pcb, const char *filename, char *const argv[], char *const envp[]);
 */
 void context_uload(PCB *pcb, const char *filename, char *const argv[], char *const envp[]) {
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", (void *)entry);
  // Context* ucontext(AddrSpace *as, Area kstack, void *entry);
  // 参数as用于限制用户进程可以访问的内存, 我们在下一阶段才会使用, 目前可以忽略它; 
  pcb->cp = ucontext(NULL, (Area) { pcb->stack, pcb->stack + sizeof(PCB) }, (void *)entry);

  // 很自然参数和环境变量的传递就需要由操作系统来负责. 最适合存放参数和环境变量的地方就是用户栈了, 
  // 因为在首次切换到用户进程的时候, 用户栈上的内容就已经可以被用户进程访问. 
  // 于是操作系统在加载用户进程的时候, 还需要负责把argc/argv/envp以及相应的字符串放在用户栈中, 
  // 并把它们的存放方式和位置作为和用户进程的约定之一, 这样用户进程在_start中就可以根据约定访问它们了.

  // 按照 C 语言的惯例，这两组指针数组argv和envp都以一个 NULL 指针作为结束标志，你需要自己写个小循环来数
  // 内核往用户栈上摆放的所有 argv/envp 所指向的字符串数据都是标准的 C‐字符串——即每个字符串都是以一个字节 '\0'（NUL）结尾的。
  
  int argc = 0;
  uint8_t *ustack_end = (uint8_t *)heap.end;

  while(argv[argc] != NULL) {
    size_t len = strlen(argv[argc]) + 1; // 包括结尾的 '\0'
    ustack_end -= len;
    memcpy(ustack_end, argv[argc], len);
    argc++;
  }

  int envpc = 0;
  while (envp[envpc] != NULL) {
    size_t len = strlen(envp[envpc]) + 1;
    ustack_end -= len;
    memcpy(ustack_end, envp[envpc], len);
    envpc++;
  }

  uint8_t *string_area = ustack_end;
  // 将NULL写进地址空间中
  uintptr_t zero = 0;
  
  // 一部分是字符串区域(string area), 另一部分是argv/envp这两个字符串指针数组
  // 数组中的每一个元素是一个字符串指针, 而这些字符串指针都会指向字符串区域中的某个字符串. 
  size_t plen = sizeof(uintptr_t);
  uintptr_t string_addr = 0;

  for (int i = envpc; i >= 0; i--) {
    ustack_end -= plen;
    string_addr = (uintptr_t)string_area;
    if (envp[i]) {
      memcpy(ustack_end, &string_addr, plen);
      string_area += strlen(envp[i]) + 1;
    }
    else memcpy(ustack_end, &zero, plen);

    printf("push in stack: sp:0x%x sp_save:0x%x <-> string_area:0x%x", (uintptr_t)ustack_end, (uintptr_t)(*(uintptr_t*)ustack_end), string_addr);
    if ((uintptr_t)(*ustack_end) != 0) printf(" string: %s\n", (char *)((uintptr_t)(*(uintptr_t*)ustack_end)));
    else printf("\n");
  }
  
  for (int i = argc; i >= 0; i--) {
    ustack_end -= plen;
    string_addr = (uintptr_t)string_area;
    if (argv[i]) {
      memcpy(ustack_end, &string_addr, plen);
      string_area += strlen(argv[i]) + 1;
    }
    else memcpy(ustack_end, &zero, plen);

    printf("push in stack: sp:0x%x sp_save:0x%x <-> string_area:0x%x", (uintptr_t)ustack_end, (uintptr_t)(*(uintptr_t*)ustack_end), string_addr);
    if ((uintptr_t)(*ustack_end) != 0) printf(" string: %s\n", (char *)((uintptr_t)(*(uintptr_t*)ustack_end)));
    else printf("\n");
  }
  
  ustack_end -= sizeof(int);
  memcpy(ustack_end, &argc, sizeof(int));

  // debug
  printf("context_uload argc: sp:0x%x sp_save:%d\n", (uintptr_t)ustack_end ,*(int *)ustack_end);
  for (int i = 0; i < argc; i++) {
    uintptr_t *addr = (uintptr_t *)(ustack_end + sizeof(int) + sizeof(uintptr_t) * i);
    printf("context_uload argv[%d]: sp:0x%x 0x%x --> %s\n", i, (uintptr_t)addr, (uintptr_t)(*addr), (char *)(*addr));
  }
  for (int i = 0; i < envpc; i++) {
    uintptr_t *addr = (uintptr_t *)(ustack_end + sizeof(int) + sizeof(uintptr_t) * (argc + 1 + i));
    printf("context_uload envp[%d]: sp:0x%x 0x%x --> %s\n", i, (uintptr_t)addr, (uintptr_t)(*addr), (char *)(*addr));
  }

  //操作系统将argc/argv/envp及其相关内容放置到用户栈上, 然后将GPRx设置为argc所在的地址. 
  pcb->cp->GPRx = (uintptr_t)ustack_end;
}