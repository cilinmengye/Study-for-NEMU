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

static uintptr_t loader(PCB *pcb, const char *filename) {
  //TODO();
  Elf_Ehdr elf_header;
  size_t getSize = ramdisk_read(&elf_header, 0, sizeof(elf_header));
  assert(getSize == sizeof(elf_header));
  printf("%x", *(uint32_t *)elf_header.e_ident);
  assert(*(uint32_t *)elf_header.e_ident == 0x7f454c46);
  
  assert(elf_header.e_machine == EXPECT_TYPE);
  
  Elf_Phdr program_header;
  for (uint16_t i = 0; i < elf_header.e_phnum; i++){
    getSize = ramdisk_read(&program_header, elf_header.e_phoff + i * sizeof(program_header), sizeof(program_header));
    assert(getSize == sizeof(program_header));
    if(program_header.p_type != PT_LOAD)
      continue;
    ramdisk_read((void *)program_header.p_vaddr, program_header.p_offset, program_header.p_filesz);
    if (program_header.p_memsz > program_header.p_filesz)
      memset((void *)(program_header.p_memsz + program_header.p_filesz), 0, program_header.p_memsz - program_header.p_filesz);
  }

  return elf_header.e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", entry);
  ((void(*)())entry) ();
}

