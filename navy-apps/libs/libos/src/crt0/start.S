.globl  _start
_start:

#if defined(__ISA_AM_NATIVE__)

  movq $0, %rbp
  // (rsp + 8) should be multiple of 16 when
  // control is transfered to the function entry point.
  // See amd64 ABI manual for more details
  andq $0xfffffffffffffff0, %rsp
  movq %rax, %rdi
  call call_main

#elif defined(__ISA_X86_64__)

  movq $0, %rbp
  movq %rax, %rsp
  movq %rax, %rdi
  call call_main

#elif defined(__ISA_X86__)

  movl $0, %ebp
  call call_main

#elif defined(__ISA_MIPS32__)

  move $fp, $zero
  jal call_main

#elif defined(__riscv)
  mv s0, zero
  // Nanos-lite和Navy作了一项约定: Nanos-lite把栈顶位置设置到GPRx中,
  // 然后由Navy里面的_start来把栈顶位置真正设置到栈指针寄存器中.

  // 根据这一约定, 你还需要修改Navy中_start的代码, 把argc的地址作为参数传递给call_main(). 
  // 然后修改call_main()的代码, 让它解析出真正的argc/argv/envp:
  mv sp, a0
  jal call_main

#elif defined(__ISA_LOONGARCH32R__)

  move $fp, $zero
  bl call_main

#else
#error Unsupported ISA
#endif
