#include <proc.h>
#include <elf.h>

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
  int fd = fs_open(filename, 0, 0);

  Elf_Ehdr elf_header;
  size_t getSize = fs_read(fd, &elf_header, sizeof(elf_header));
  printf("base offset: %d %d\n", 0, 400143 + 0);
  //size_t getSize = ramdisk_read(&elf_header, 0, sizeof(elf_header));
  assert(getSize == sizeof(elf_header));
  assert(*(uint32_t *)elf_header.e_ident == 0x464c457f);
  assert(elf_header.e_machine == EXPECT_TYPE);
  
  Elf_Phdr program_header;
  fs_lseek(fd, elf_header.e_phoff, 0);
  for (uint16_t i = 0; i < elf_header.e_phnum; i++){
    getSize = fs_read(fd, &program_header, sizeof(program_header));
    printf("base offset: %d\n", 400143 + elf_header.e_phoff + i * sizeof(program_header));
    //getSize = ramdisk_read(&program_header, elf_header.e_phoff + i * sizeof(program_header), sizeof(program_header));
    assert(getSize == sizeof(program_header));
    if(program_header.p_type != PT_LOAD)
      continue;
    
    fs_lseek(fd, program_header.p_offset, 0);
    fs_read(fd, (void *)program_header.p_vaddr, program_header.p_filesz);
    printf("base offset: %d\n", 400143 + program_header.p_offset);
    //ramdisk_read((void *)program_header.p_vaddr, program_header.p_offset, program_header.p_filesz);

    if (program_header.p_memsz > program_header.p_filesz)
      memset((void *)(program_header.p_vaddr + program_header.p_filesz), 0, program_header.p_memsz - program_header.p_filesz);
  }
  fs_close(fd);
  return elf_header.e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", entry);
  ((void(*)())entry) ();
}

