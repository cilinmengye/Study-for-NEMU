# ICS2023 Programming Assignment

This project is an important part of the [One Chip for One Life project](https://ysyx.oscc.cc/) and the programming assignment of the class ICS
in Department of Computer Science and Technology, Nanjing University.

For the guide of this programming assignment,
refer to https://nju-projectn.github.io/ics-pa-gitbook/ics2023/

To initialize, run
```bash
bash init.sh subproject-name
```
See `init.sh` for more details.

The following subprojects/components are included. Some of them are not fully implemented.
* [NEMU](https://github.com/NJU-ProjectN/nemu)
* [Abstract-Machine](https://github.com/NJU-ProjectN/abstract-machine)
* [Nanos-lite](https://github.com/NJU-ProjectN/nanos-lite)
* [Navy-apps](https://github.com/NJU-ProjectN/navy-apps)


# PA1 - 开天辟地的篇章: 最简单的计算机

- task PA1.1: 实现单步执行, 打印寄存器状态, 扫描内存
- task PA1.2: 实现算术表达式求值
- task PA1.3: 实现所有要求, 提交完整的实验报告

## NEMU是什么？

**NJU EMUlator**

一款经过简化的全系统模拟器,它模拟了一个硬件的世界, 你可以在这个硬件世界中执行程序.

即我们要做的事情是：编写一个用来执行其它程序的程序! 

![image-20240626121308160](./NEMU.assets/image-20240626121308160.png)

**在NEMU中, 每一个硬件部件都由一个程序相关的数据对象来模拟, 例如变量, 数组, 结构体等** 

* Decode 结构体模拟在译码时需要保存的数据：

  ```
  typedef struct Decode {
    vaddr_t pc;
    vaddr_t snpc; // static next pc
    vaddr_t dnpc; // dynamic next pc
    uint32_t val; // 指令
    IFDEF(CONFIG_ITRACE, char logbuf[128]);
  } Decode
  ```

* 内存用数组模拟，以字节为单位进行编址

  ```
  static uint8_t pmem[CONFIG_MSIZE]  __attribute((aligned(4096))）
  ```

* cpu用结构体模拟，cpu寄存器用数组模拟：

  ```
  extern CPU_state cpu;
  //nemu/src/isa/riscv32/include/isa-def.h
  typedef struct {
    word_t mtvec; //异常入口地址寄存器
    vaddr_t mepc; //发生异常时保存pc的寄存器
    word_t mstatus; //存放处理器的状态
    word_t mcause; //异常号寄存器
  } MUXDEF(CONFIG_RV64, riscv64_CSRS, riscv32_CSRS);
  
  typedef struct {
    word_t gpr[MUXDEF(CONFIG_RVE, 16, 32)];
    vaddr_t pc;
    MUXDEF(CONFIG_RV64, riscv64_CSRS, riscv32_CSRS) csrs; //控制状态寄存器(CSR寄存器)
  } MUXDEF(CONFIG_RV64, riscv64_CPU_state, riscv32_CPU_state);
  ```

  CPU的结构是ISA相关的

  ```
  //nemu/src/isa/riscv32/reg.c
  const char *regs[] = {
    "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
    "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
    "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
    "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
  };
  
  const char *csrs[] = {
    "mtvec", "mepc", "mstatus", "mcause"
  };
  ```

**而对这些部件的操作则通过对相应数据对象的操作来模拟. 例如NEMU中使用数组来模拟内存, 那么对这个数组进行读写则相当于对内存进行读写.**

## 最简单的计算机

```
while (1) {
  从PC指示的存储器位置取出指令;
  执行指令;
  更新PC;
}
```

为了方便叙述, 我们将在**NEMU中模拟的计算机称为"客户(guest)计算机"**, 在**NEMU中运行的程序称为"客户程序"**.

**NEMU主要由4个模块构成: monitor, CPU, memory, 设备**

将NEMU看作一个状态机，通过判断状态，来停运NEMU

```
enum { NEMU_RUNNING, NEMU_STOP, NEMU_END, NEMU_ABORT, NEMU_QUIT };

typedef struct {
  int state;
  vaddr_t halt_pc;
  uint32_t halt_ret;
} NEMUState;

extern NEMUState nemu_state;

//nemu/include/utils.h
```



## RTFSC

/nemu/include/cpu/decode.h 理解编写译码的代码

/nemu/include/cpu/ifetch.h 理解编写取指的代码

<hr>


> x86的物理内存是从0开始编址的, 但对于一些ISA来说却不是这样, 例如mips32和riscv32的物理地址均从`0x80000000`开始. 

/nemu/include/memory/host.h 理解编写访问内存的代码

/nemu/include/memory/paddr.h 编写的是将客户程序使用的物理地址转换为真正能够访问的数组地址

/nemu/include/memory/vaddr.h 是关于虚拟地址的，不用管。

<hr>


nemu/scripts 编译，调试，运行，设置（如设置内存大小，设置是否日志打印等）nemu的makefile脚本

<hr>


/nemu/include/debug.h 理解编写调试用的宏代码



## 基础设施: 简易调试器

基础设施是指支撑项目开发的各种工具和手段. 

简易调试器(Simple Debugger, sdb)是NEMU中一项非常重要的基础设施. 我们知道NEMU是一个用来执行其它客户程序的程序, 这意味着, NEMU可以随时了解客户程序执行的所有信息. 然而这些信息对外面的调试器(例如GDB)来说, 是不容易获取的. 

* 继续运行

  c

* 退出

​	q

* 单步执行

  si [N]

  让程序单步执行`N`条指令后暂停执行,当`N`没有给出时, 缺省为`1`

* 打印寄存器状态 

​	info r

* 打印监视点信息

  info w

* 扫描内存

  x N EXPR

  求出表达式`EXPR`的值, 将结果作为起始内存地址, 以十六进制形式输出连续的`N`个4字节

* 表达式求值

  p EXPR

  求出表达式`EXPR`的值

* 设置监视点

  w EXPR

  当表达式`EXPR`的值发生变化时, 暂停程序执行

* 删除监视点

​	d N

​	删除序号为`N`的监视点 

### 用户交互性实现

* readline 

  为了让简易调试器易于使用, NEMU通过`readline`库与用户交互, 使用`readline()`函数**从键盘上读入命令**.

  `readline()`提供了"行编辑"的功能, 最常用的功能就是**通过上, 下方向键翻阅历史记录**. 

* getopt_long

  用于解析命令行选项和参数,不仅支持短选项（如 `-h`）还支持长选项（如 `--help`），以及带参数的选项（`-f filename` 或 `--file=filename`）

* strtok

​	用于用来解析用户以空格或其他符号分隔的命令，可将字符串分割成一系列的tokens，通常根据一个或多个分隔符来分割。

​	如：`info w`以空格作为分隔符，可分割出`info`和`w`两个token

### 表达式求值实现

需要支持识别十六进制整数(`0x80100000`)，小括号， 访问寄存器(`$a0`)，指针解引用(第二个`*`), 访问变量(`number`)

解决方法为：

#### 词法分析

* 为算术表达式中的各种token类型添加规则，用正则表达式识别出token

  ```
  enum {
    TK_NOTYPE = 256, TK_EQ, TK_NUMBER, TK_NEGATIVE, TK_HEX, TK_REG, TK_UNEQ, TK_AND, TK_POINTER
  };
  static struct rule {
    const char *regex;
    int token_type;
  } rules[] = {
    {" +", TK_NOTYPE},    // spaces
    {"\\+", '+'},         // plus
    {"==", TK_EQ},        // equal
    {"\\-", '-'},         // sub
    {"\\(", '('},           // left parenthesis
    {"\\)", ')'},           // right parenthesis
    {"\\*", '*'},         // multiply
    {"/", '/'},           // division
    {"0x[0-9a-fA-F]+", TK_HEX},   // <hexadecimal-number>  
    {"(0u?|[1-9][0-9]*u?)", TK_NUMBER}, // decimal integer
    {"\\$[0-9a-zA-Z]+", TK_REG}, // <reg_name> 
    {"!=", TK_UNEQ},    // unequal
    {"&&", TK_AND},   // and 
  };
  ```

* 在成功识别出token后, 将token的信息依次记录到`tokens`数组中.

  ```
  typedef struct token {
    int type;
    char str[32];
  } Token;
  ```

> C 语言中使用正则表达式（regex）可以通过 POSIX 标准库中的 regex.h 提供的功能实现
>
> `regex.h` 提供的主要函数
>
> - **`regcomp`**: 编译正则表达式。
> - **`regexec`**: 执行正则表达式匹配。
> - **`regfree`**: 释放正则表达式结构。
> - **`regerror`**: 获取错误信息。
>
> 基本使用流程
>
> 1. **编译正则表达式**: 使用 `regcomp` 将正则表达式编译成 `regex_t` 结构。
> 2. **执行匹配**: 使用 `regexec` 进行匹配操作，返回匹配结果。
> 3. **释放资源**: 使用 `regfree` 释放 `regex_t` 结构所占用的资源。



> POSIX（Portable Operating System Interface for Unix）是一个定义了一组 API 和标准的系统接口，提供了跨各种 Unix 类操作系统（以及类 Unix 系统，如 Linux 和 macOS）的一致性。POSIX 的标准由 IEEE 制定，包括文件系统操作、进程管理、线程、信号处理、输入输出操作等功能。POSIX 标准确保了应用程序可以在不同的 Unix 类系统上进行移植，而不需要对代码进行大规模的修改。

#### 递归求值

依据算术表达式的归纳定义:

```
<expr> ::= <number>    # 一个数是表达式
  | "(" <expr> ")"     # 在表达式两边加个括号也是表达式
  | <expr> "+" <expr>  # 两个表达式相加也是表达式
  | <expr> "-" <expr>  # 接下来你全懂了
  | <expr> "*" <expr>
  | <expr> "/" <expr>
```

递归地求解表达式的值

其中更多的细节是：

* 去除括号：使用栈进行括号匹配，匹配成功则说明可以去除括号，或者说明表达式非法

* 寻找主运算符：要正确地对一个长表达式进行分裂，就是要找到它的主运算符

  	* 主运算符的优先级在表达式中是最低的
  	
  	* 当有多个运算符的优先级都是最低时, 根据结合性, 最后被结合的运算符才是主运算符
  	* 出现在一对括号中的token不是主运算符
  	
  	* 非运算符的token不是主运算符

### 监视点和断点实现

**监视点**

用链表实现建立监视点池，每一个节点都是一个结构体，其中保存着下一个节点的指针，监视点号，要监视的表达式旧值，要监视的表达式。

每次执行完一次cpu_exec(1)时，扫描整个监视点池，判断表达式与旧值是否相同。

**断点**

```
w $pc == ADDR
```

实现方式如上，监视寄存器pc是否等于某个地址，只有当到达要设置断点的地址时,表达式值才为true，其余情况都为fasle

### 调试器是如何管控NEMU模拟的客户计算机的？

```
void cpu_exec(uint64_t n);
```

上述代码包含了`  从PC指示的存储器位置取出指令;执行指令;更新PC;`这三个过程

调试器之所以能够管控客户计算机，实际上是掌控了外层的while循环，即调试器能够决定在什么情况下执行cpu_exec

在NEMU的main函数中：

```c
///nemu/src/nemu-main.c
int main(int argc, char *argv[])
{
  /* Initialize the monitor. */
#ifdef CONFIG_TARGET_AM
  am_init_monitor();
#else
  init_monitor(argc, argv);
#endif
    /* Start engine. */
  engine_start();

  return is_exit_status_bad();
}
```

am_init_monitor和init_monitor做的事情都是初始化工作，如初始化内存，初始化设备等。不同的是init_monitor是在非批处理模式下运行的，非批处理模式下是需要打印调试的，所以还需要做一些关于调试的初始化。

在engine_start()中根据是否为批处理模式，来决定是否进入调试器的循环中。

```
void engine_start() {
#ifdef CONFIG_TARGET_AM
  cpu_exec(-1);
#else
  /* Receive commands from user. */
  sdb_mainloop();
#endif
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }
  ...
}
```

# PA2 - 简单复杂的机器: 冯诺依曼计算机系统

- task PA2.1: 实现更多的指令, 在NEMU中运行大部分`cpu-tests`
- task PA2.2: 实现klib和基础设施
- task PA2.3: 运行FCEUX, 提交完整的实验报告

上面PA1中我们还没有实现指令，还只是图灵机(TRM)

## 取指与译码

```
//nemu/src/cpu/cpu-exec.c
static void exec_once(Decode *s, vaddr_t pc) {
  s->pc = pc;
  s->snpc = pc;
  isa_exec_once(s);
  cpu.pc = s->dnpc;
}
```

开始时，让Decode结构体中的pc，snpc均等于cpu.pc

保证cpu.pc一定是正确的pc值。

```
int isa_exec_once(Decode *s) {
  s->isa.inst.val = inst_fetch(&s->snpc, 4);
  IFDEF(CONFIG_IRINGTRACE, iringbuf_get(*s));
  return decode_exec(s);
}
```

```
static inline uint32_t inst_fetch(vaddr_t *pc, int len) {
  uint32_t inst = vaddr_ifetch(*pc, len);
  (*pc) += len;
  return inst;
}
```

inst_fetch函数完成取指，并让snpc更新，decode_exec()开始译码

在取指完成后，snpc更新，在译码的开始，让dnpc等于snpc

> snpc是下一条静态指令, 而dnpc是下一条动态指令. 对于顺序执行的指令, 它们的snpc和dnpc是一样的; 
>
> 但对于跳转指令, snpc和dnpc就会有所不同, dnpc应该指向跳转目标的指令
>
> 指令如果要对下一个pc值产生影响，我们会更新dnpc，指令执行完成后让cpu.pc = dnpc

译码的过程是根据 RISCV32的ISA手册，其指令定义了：[**R型指令格式**，**I型指令格式**，**S型指令格式**，**B型指令格式**，**U型指令格式**，**J型指令格式**](https://www.cnblogs.com/cilinmengye/p/18127248)

我们可以更加指令的操作码（操作码固定在一条指令的0~6位）和功能码区分出指令格式，进而得到源操作数，目的操作数等内容

![image](https://img2024.cnblogs.com/blog/2641775/202404/2641775-20240410212630805-1050352269.png)

## 运行时环境

大多数编程语言都有**某种形式的运行时系统，为程序的运行提供环境。**

这种环境可以解决许多问题，包括应用程序内存的管理、程序如何访问变量、在过程之间传递参数的机制、与操作系统 (OS) 的接口，动态链接库等。

编译器根据特定的运行时系统做出假设以生成正确的代码。通常，运行时系统将负责设置和管理堆栈，并可能包括垃圾收集、线程或语言内置的其他动态功能。

**运行时系统行为的一个可能定义是“任何不直接归因于程序本身的行为”**。此定义包括在函数调用之前将参数放入堆栈、并行执行相关行为以及磁盘 I/O。

### 将运行时环境封装成库函数

**运行时环境是直接位于计算机硬件之上的, 因此运行时环境的具体实现, 也是和架构相关的. **



结束运行是程序共有的需求, 为了让`n`个程序运行在`m`个架构上, 难道我们要维护`n*m`份代码? 有没有更好的方法呢?

**抽象! 我们只需要定义一个结束程序的API**, 比如`void halt()`, 它对不同架构上程序的不同结束方式进行了抽象: 程序只要调用`halt()`就可以结束运行, 而不需要关心自己运行在哪一个架构上. 

经过抽象之后，我们就可以把程序和架构解耦了: 我们只需要维护`n+m`份代码(`n`个程序和`m`个架构相关的`halt()`), 而不是之前的`n*m`.

### AM(abstract-machine)

应用程序的运行都需要运行时环境的支持,更复杂的应用程序对运行时环境必定还有其它的需求。（需要和用户进行交互, 至少需要运行时环境提供输入输出的支持）

如果我们把这些需求都收集起来, 将它们抽象成统一的API提供给程序, 这样我们就得到了一个可以支撑各种程序运行在各种架构上的库了! 

由于这组统一抽象的API**代表了程序运行对计算机的需求, 所以我们把这组API称为抽象计算机.**

AM(Abstract machine)项目就是这样诞生的. 作为一个向程序提供运行时环境的库, AM根据程序的需求把库划分成以下模块

```
AM = TRM + IOE + CTE + VME + MPE
```

- TRM(Turing Machine) - 图灵机, 最简单的运行时环境, 为程序提供基本的计算能力
  - `Area heap`结构用于指示堆区的起始和末尾
  - `void putch(char ch)`用于输出一个字符
  - `void halt(int code)`用于结束程序的运行
  - `void _trm_init()`用于进行TRM相关的初始化工作

- IOE(I/O Extension) - 输入输出扩展, 为程序提供输出输入的能力
- CTE(Context Extension) - 上下文扩展, 为程序提供上下文管理的能力
- ~~VME(Virtual Memory Extension) - 虚存扩展, 为程序提供虚存管理的能力~~
- ~~MPE(Multi-Processor Extension) - 多处理器扩展, 为程序提供多处理器通信的能力 (MPE超出了ICS课程的范围, 在PA中不会涉及)~~

> 我们编写的程序依赖运行时环境，如常见的运行环境有[Java](https://zh.wikipedia.org/wiki/Java)运行环境Java Runtime Environment（JRE）
>
> 若没有JRE，我们编写的java许多函数都调用不了。
>
> 所以在这里我们是在AM的基础之上编写代码

<hr>


* NEMU/abstract-machine/am/include /arch /riscv.h  

  RISCV32架构下的上下文数据结构

  ```
  #define NR_REGS 32
  struct Context {
    // TODO: fix the order of these members to match trap.S
    uintptr_t gpr[NR_REGS], mcause, mstatus, mepc;
    void *pdir;
  };
  
  #ifdef __riscv_e
  #define GPR1 gpr[15] // a5
  #else
  #define GPR1 gpr[17] // a7
  #endif
  
  #define GPR2 gpr[10] // a0
  #define GPR3 gpr[11] // a1
  #define GPR4 gpr[12] // a2
  #define GPRx gpr[10] // a0
  
  #endif
  ```

* NEMU/abstract-machine/am/include/am.h 

  包括AM API实现和数据结构等

  ```
  // Memory area for [@start, @end)
  typedef struct {
    void *start, *end;
  } Area;
  
  // Arch-dependent processor context
  typedef struct Context Context;
  typedef struct {
    enum {
      EVENT_NULL = 0,
      EVENT_YIELD, EVENT_SYSCALL, EVENT_PAGEFAULT, EVENT_ERROR,
      EVENT_IRQ_TIMER, EVENT_IRQ_IODEV,
    } event;
    uintptr_t cause, ref;
    const char *msg;
  } Event;
  // ----------------------- TRM: Turing Machine -----------------------
  extern   Area        heap;
  void     putch       (char ch);
  void     halt        (int code) __attribute__((__noreturn__));
  
  // -------------------- IOE: Input/Output Devices --------------------
  bool     ioe_init    (void);
  void     ioe_read    (int reg, void *buf);
  void     ioe_write   (int reg, void *buf);
  void     _trm_init   (void);
  
  // ---------- CTE: Interrupt Handling and Context Switching ----------
  bool     cte_init    (Context *(*handler)(Event ev, Context *ctx));
  void     yield       (void);
  bool     ienabled    (void);
  void     iset        (bool enable);
  Context *kcontext    (Area kstack, void (*entry)(void *), void *arg);
  
  // ----------------------- VME: Virtual Memory -----------------------
  bool     vme_init    (void *(*pgalloc)(int), void (*pgfree)(void *));
  void     protect     (AddrSpace *as);
  void     unprotect   (AddrSpace *as);
  void     map         (AddrSpace *as, void *vaddr, void *paddr, int prot);
  Context *ucontext    (AddrSpace *as, Area kstack, void *entry);
  
  // ---------------------- MPE: Multi-Processing ----------------------
  bool     mpe_init    (void (*entry)());
  int      cpu_count   (void);
  int      cpu_current (void);
  int      atomic_xchg (int *addr, int newval);
  ```

  >  可以看到我们提供了：堆，结束程序，异常处理，切换上下文，I/O输入输出的运行环境

* NEMU/abstract-machine/klib/include/klib.h 

  基于NEMU模拟的硬件，我们也实现常用的库string.h，stdlib.h，stdio.h，assert.h

* NEMU/abstract-machine/scripts 

  构建/运行二进制文件/镜像的Makefile

<hr>


我们对异常处理需要硬件的支持，因为在NEMU中硬件是我们模拟出来的。

在实现运行时环境时，我们获得硬件支持的方法为**插入[内联汇编](http://www.ibiblio.org/gferg/ldp/GCC-Inline-Assembly-HOWTO.html)语句**和在实现指令时额外对模拟的硬件进行操作。

如对于halt()的实现：

```
# define nemu_trap(code) asm volatile("mv a0, %0; ebreak" : :"r"(code))

void halt(int code) {
  nemu_trap(code);

  // should not reach here
  while (1);
}
```

`nemu_trap()`宏还会把一个标识结束的结束码移动到通用寄存器中

```
#define NEMUTRAP(thispc, code) set_nemu_state(NEMU_END, thispc, code)
  // void set_nemu_state(int state, vaddr_t pc, int halt_ret) {
  //   difftest_skip_ref();
  //   nemu_state.state = state;
  //   nemu_state.halt_pc = pc;
  //   nemu_state.halt_ret = halt_ret;
  // }
INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak , N, NEMUTRAP(s->pc, R(10)));
```

通用寄存器中的值将会作为参数传给`set_nemu_state()`, 将`halt()`中的结束码设置到NEMU的monitor中, monitor将会根据结束码来报告程序结束的原因. 

<hr>


在让NEMU运行客户程序之前, 我们需要将客户程序的代码编译成可执行文件. 

我们需要在GNU/Linux下根据AM的运行时环境编译出能够在`$ISA-nemu`这个新环境中运行的可执行文件

解决这个问题的方法是[交叉编译](http://en.wikipedia.org/wiki/Cross_compiler).

AM的框架代码已经把相应的配置准备好了, 上述编译和链接选项主要位于`abstract-machine/Makefile` 以及`abstract-machine/scripts/`目录下的相关`.mk`文件中. 

编译生成一个可以在NEMU的运行时环境上运行的程序的过程大致如下:

- gcc将`$ISA-nemu`的AM实现源文件编译成目标文件, 然后通过ar将这些目标文件作为一个库, 打包成一个归档文件`abstract-machine/am/build/am-$ISA-nemu.a`
- gcc把应用程序源文件(如`am-kernels/tests/cpu-tests/tests/dummy.c`)编译成目标文件
- 通过gcc和ar把程序依赖的运行库(如`abstract-machine/klib/`)也编译并打包成归档文件
- 根据Makefile文件`abstract-machine/scripts/$ISA-nemu.mk`中的指示, 让ld根据链接脚本`abstract-machine/scripts/linker.ld`, 将上述目标文件和归档文件链接成可执行文件

<hr>


### 在拥有了AM后，我们的客户计算机是如何开始运行的？

1. 第一条指令从`abstract-machine/am/src/$ISA/nemu/start.S`开始, 设置好栈顶之后就跳转到`abstract-machine/am/src/platform/nemu/trm.c`的`_trm_init()`函数处执行.

   ```
   .section entry, "ax"
   .globl _start
   .type _start, @function
   
   _start:
     mv s0, zero
     la sp, _stack_pointer
     jal _trm_init
   ```

2. 在`_trm_init()`中调用`main()`函数执行程序的主体功能, `main()`函数还带一个参数, 目前我们暂时不会用到, 后面我们再介绍它.

   ```
   void _trm_init() {
     int ret = main(mainargs);
     halt(ret);
   }
   ```

   > 这个main并不是NEMU的main,而客户程序的main

3. 从`main()`函数返回后, 调用`halt()`结束运行.

## 输入输出

输入输出都是通过访问I/O设备来完成的.

事实上, 只要向设备发送一些有意义的数字信号, 设备就会按照这些信号的含义来工作.

设备也有自己的状态寄存器(相当于CPU的寄存器), 也有自己的功能部件(相当于CPU的运算器). 当然不同的设备有不同的功能部件, 例如键盘有一个把按键的模拟信号转换成扫描码的部件, 而VGA则有一个把像素颜色信息转换成显示器模拟信号的部件. 



设备是用来进行输入输出的, 所谓的访问设备, 说白了就是从设备获取数据(输入), 比如从键盘控制器获取按键扫描码；

或者是向设备发送数据(输出), 比如向显存写入图像的颜色信息.

除了纯粹的数据读写之外, 我们还需要对设备进行控制。比如需要获取键盘控制器的状态, 查看当前是否有按键被按下; 或者是需要有方式可以查询或设置VGA控制器的分辨率. 

**在程序看来, 访问设备 = 读出数据 + 写入数据 + 控制状态.**

<hr>


既然设备也有寄存器, 一种最简单的方法就是**把设备的寄存器作为接口, 让CPU来访问这些寄存器.**

给设备中允许CPU访问的寄存器逐一编号, 然后通过指令来引用这些编号. 

设备中可能会有一些私有寄存器, 它们是由设备自己维护的, 它们没有这样的编号, CPU不能直接访问它们.



这就是所谓的I/O编址方式, 因此这些编号也称为设备的地址. 常用的编址方式有两种.

* 端口I/O

  CPU使用专门的I/O指令对设备进行访问, 并把设备的地址称作端口号. 有了端口号以后, 在I/O指令中给出端口号, 就知道要访问哪一个设备寄存器了. 

* 内存映射I/O

  通过不同的物理内存地址给设备编址的. 这种编址方式将一部分物理内存的访问"重定向"到I/O地址空间中, CPU尝试访问这部分物理内存的时候, 实际上最终是访问了相应的I/O设备, CPU却浑然不知.

作为RISC架构, riscv32是采用内存映射I/O的编址方式. 

### NEMU模拟内存映射I/O

```
/*框架代码为映射定义了一个结构体类型IOMap：名字, 映射的起始地址和结束地址, 映射的目标空间, 以及一个回调函数.*/
typedef struct {
  const char *name;
  // we treat ioaddr_t as paddr_t here
  paddr_t low;
  paddr_t high;
  void *space;
  io_callback_t callback;
} IOMap;
```

内存映射I/O的模拟:

首先我们编写的客户程序被编译链接成可执行文件后由NEMU识别执行。其中当指令识别出是访存时，调用` vaddr_read`,` vaddr_write`(其本质上是调用`paddr_read()`和`paddr_write()`)

`paddr_read()`和`paddr_write()`会判断地址`addr`落在物理内存空间还是设备空间, 若落在物理内存空间, 就会通过`pmem_read()`和`pmem_write()`来访问真正的物理内存; 

否则就通过`map_read()`和`map_write()`来访问相应的设备. 

```c
/*
 * 其中map_read()和map_write()用于将地址addr映射到map所指示的目标空间, 并进行访问. 
 * 访问时, 可能会触发相应的回调函数, 对设备和目标空间的状态进行更新.
 * 由于NEMU是单线程程序, 因此只能串行模拟整个计算机系统的工作, 每次进行I/O读写的时候, 才会调用设备提供的回调函数(callback). 
 */
word_t map_read(paddr_t addr, int len, IOMap *map) {
  assert(len >= 1 && len <= 8);
  check_bound(map, addr);
  paddr_t offset = addr - map->low;
  invoke_callback(map->callback, offset, len, false); // prepare data to read
  word_t ret = host_read(map->space + offset, len);
  IFDEF(CONFIG_DTRACE, dtraceRead_display(map->space + offset, len, map));
  return ret;
}

void map_write(paddr_t addr, int len, word_t data, IOMap *map) {
  assert(len >= 1 && len <= 8);
  check_bound(map, addr);
  paddr_t offset = addr - map->low;
  host_write(map->space + offset, len, data);
  IFDEF(CONFIG_DTRACE, dtraceWrite_display(map->space + offset, len, data, map));
  invoke_callback(map->callback, offset, len, true);
}
```

当我们的客户程序往设备的内存中写数据时，可以看到会触发回调函数，做到控制设备状态的作用。

### NEMU模拟硬件设备

NEMU实现了串口, 时钟, 键盘, VGA, 声卡, 磁盘, SD卡七种设备,

NEMU使用SDL库来实现设备的模拟, `nemu/src/device/device.c`含有和SDL库相关的代码.

`abstract-machine/am/include/amdev.h`中定义了常见设备的"抽象寄存器"编号和相应的结构. 这些定义是架构无关的, 每个架构在实现各自的IOE API时, 都需要遵循这些定义(约定). 

特别地, NEMU作为一个平台, 设备的行为是与ISA无关的, 因此我们只需要在`abstract-machine/am/src/platform/nemu/ioe/`目录下实现一份IOE, 来供NEMU平台的架构共享. 

> SDL（Simple DirectMedia Layer）是一个跨平台的多媒体开发库，主要用于访问底层硬件（如图形、音频、输入设备等 ，非常适合开发游戏和多媒体应用。

总体来说思路是一样的，我们在NEMU使用SDL库通过对物理世界的访问来模拟NEMU客户计算机中的设备

比如：当我在物理世界按下键盘一个键，我们编写的SDL库程序可以知道我按下了哪个键，并将这个信息保存到内存映射I/O的内存中。

然后我的客户程序通过运行时环境提供的库，调用相应API访问到内存，读/写设备的信息。

运行时环境称为IOE，其被抽象成了3个函数`ioe_init`,`ioe_read`,`ioe_write`

`ioe_read()`和`ioe_write()`都是通过抽象寄存器的编号索引到一个处理函数（因为不同设备读和写操作可能会有不同，所以我们对不同设备实现了读和写操作）, 然后调用它. 

```c
// /abstract-machine/am/src/platform/nemu/ioe/ioe.c
typedef void (*handler_t)(void *buf); //定义了一个函数指针类型 handler_t
static void *lut[128] = { //实现了一个使用查找表（lookup table，简称 lut）.lut 是一个大小为 128 的指针数组，它存储了不同的函数指针。下面的处理函数都是我们要到/abstract-machine/am/src/platform/nemu/ioe/xxx.c中实现的。
  //如：AM_TIMER_CONFIG，AM_TIMER_RTC之类的是一个enum枚举常量，它们在/abstract-machine/am/include/amdev.h 被定义了，那么下面这个语法表示enum{AM_TIMER_CONFIG} [0] = __am_timer_config的感觉
  [AM_TIMER_CONFIG] = __am_timer_config,
  [AM_TIMER_RTC   ] = __am_timer_rtc,
  [AM_TIMER_UPTIME] = __am_timer_uptime,
  [AM_INPUT_CONFIG] = __am_input_config,
  [AM_INPUT_KEYBRD] = __am_input_keybrd,
  [AM_GPU_CONFIG  ] = __am_gpu_config,
  [AM_GPU_FBDRAW  ] = __am_gpu_fbdraw,
  [AM_GPU_STATUS  ] = __am_gpu_status,
  [AM_UART_CONFIG ] = __am_uart_config,
  [AM_AUDIO_CONFIG] = __am_audio_config,
  [AM_AUDIO_CTRL  ] = __am_audio_ctrl,
  [AM_AUDIO_STATUS] = __am_audio_status,
  [AM_AUDIO_PLAY  ] = __am_audio_play,
  [AM_DISK_CONFIG ] = __am_disk_config,
  [AM_DISK_STATUS ] = __am_disk_status,
  [AM_DISK_BLKIO  ] = __am_disk_blkio,
  [AM_NET_CONFIG  ] = __am_net_config,
};

static void fail(void *buf) { panic("access nonexist register"); }


bool ioe_init() {
  for (int i = 0; i < LENGTH(lut); i++)
    if (!lut[i]) lut[i] = fail;
  __am_gpu_init();
  __am_timer_init();
  __am_audio_init();
  return true;
}
void ioe_read (int reg, void *buf) { ((handler_t)lut[reg])(buf); }
void ioe_write(int reg, void *buf) { ((handler_t)lut[reg])(buf); }
```

``` c
/* /abstract-machine/klib/include/klib-macros.h
 * 为了方便地对这些抽象寄存器进行访问, klib中提供了io_read()和io_write()这两个宏, 
 * 它们分别对ioe_read()和ioe_write()这两个API进行了进一步的封装.
 */
//由于这个宏使用了 GNU C 中的“语句表达式”扩展（({ ... })），可以将代码块的最后一个表达式（即 __io_param）作为整个宏的返回值。
//宏 io_read 用于从指定设备寄存器 reg 读取数据，生成一个与 reg 对应的数据结构，并返回这个结构体。你可以直接使用这个结构体来访问设备的状态。
#define io_read(reg) \
  ({ reg##_T __io_param; \
    ioe_read(reg, &__io_param); \
    __io_param; })

//宏 io_write 用于向指定的设备寄存器 reg 写入数据。它会根据传入的参数生成一个与 reg 对应的数据结构，并将这个结构体通过 ioe_write 函数写入设备。
#define io_write(reg, ...) \
  ({ reg##_T __io_param = (reg##_T) { __VA_ARGS__ }; \
    ioe_write(reg, &__io_param); })
```

总之经过上述一顿操作后，我们在AM中实现了只要利用`io_read`, `io_write`传入设备的枚举常量（io_write还要传入buf，表示将buf中的内容输入到设备中）就实现了读写设备的API

如：` AM_INPUT_KEYBRD_T ev = io_read(AM_INPUT_KEYBRD);`

`io_write(AM_GPU_FBDRAW, x, y, (void *)buf, len, 1, true);`

<hr>


设备完整的运行过程：

这里我以keyboard的实现作为说明：

```c
// /nemu/src/device/keyboard.c
void init_i8042() {
  i8042_data_port_base = (uint32_t *)new_space(4); //注册内存映射I/O
  i8042_data_port_base[0] = NEMU_KEY_NONE;
  add_mmio_map("keyboard", CONFIG_I8042_DATA_MMIO, i8042_data_port_base, 4, i8042_data_io_handler);
  ...
  IFNDEF(CONFIG_TARGET_AM, init_keymap());
}
```



每个设备的具体实现可分为3大类：

1. 注册内存映射I/O

   ```c
   // /nemu/src/device/io/map.c
   
   static uint8_t *io_space = NULL; 
   static uint8_t *p_space = NULL;
   
   void init_map() {
     io_space = malloc(IO_SPACE_MAX); //我们用malloc注册了一片空间给设备进行存储数据
     assert(io_space);
     p_space = io_space;
   }
   
   uint8_t* new_space(int size) {
     uint8_t *p = p_space;
     // page aligned;
     size = (size + (PAGE_SIZE - 1)) & ~PAGE_MASK;
     p_space += size;
     assert(p_space - io_space < IO_SPACE_MAX);
     return p; // new_space做的就是给每个设备划分空间
   }
   
   // /nemu/include/device/map.h
   
   typedef struct {
     const char *name;
     // we treat ioaddr_t as paddr_t here
     paddr_t low;
     paddr_t high;
     void *space;
     io_callback_t callback;
   } IOMap;
   
   // /nemu/src/device/io/mmio.c
   
   static IOMap maps[NR_MAP] = {}; //记录注册了内存映射I/O的设备
   static int nr_map = 0;
   
   //add_mmio_map干的事情就是在我们模拟的内存上注册(占)一片空间，但是这个空间并不是用来保存数据的
   //单纯就是为了让‘访问内存就相当于访问到了设备对应的内存’
   //有点像map的感觉, map<模拟内存，设备内存>IOMAP
   void add_mmio_map(const char *name, paddr_t addr, void *space, uint32_t len, io_callback_t callback) {
     ...
     maps[nr_map] = (IOMap){ .name = name, .low = addr, .high = addr + len - 1,
       .space = space, .callback = callback };
     ...
   
     nr_map ++;
   }
   
   ```

2. 注册回调函数

   ```c
   //回调函数就是处理读/写设备时，准备数据
   //对应keyboard就是在当被读入前,调用i8042_data_io_handler从键盘缓冲栈中取下一个数据放到内存中。
   //然后再调用读的API(map_read函数就是这么实现的)
   static void i8042_data_io_handler(uint32_t offset, int len, bool is_write) {
     assert(!is_write);
     assert(offset == 0);
     i8042_data_port_base[0] = key_dequeue();
   }
   
   //io_read(AM_INPUT_KEYBRD)最终会执行keyboard的读处理函数__am_input_keybrd，对数据进行解析
   static inline uint32_t inl(uintptr_t addr) { return *(volatile uint32_t *)addr; }
   
   void __am_input_keybrd(AM_INPUT_KEYBRD_T *kbd) {
     uint32_t am_scancode = inl(KBD_ADDR);
     kbd->keydown = am_scancode & 0x8000;
     kbd->keycode = am_scancode & 0xff;
   }
   
   static uint32_t key_dequeue() {
     AM_INPUT_KEYBRD_T ev = io_read(AM_INPUT_KEYBRD);
     uint32_t am_scancode = ev.keycode | (ev.keydown ? KEYDOWN_MASK : 0);
     return am_scancode;
   }
   ```

3. 状态控制

   ```
   //不同的设备有不同的实现，总之就是维护设备该有的性质
   ```

<hr>


```c
void engine_start() { //engine_start函数为NEMU的启动核心函数，cpu_exec是cpu启动的核心函数
#ifdef CONFIG_TARGET_AM
  cpu_exec(-1);
  ...
}
//如下为简化后cpu_exec函数的主要运作代码, 其中execute是执行n步指令的核心函数
/* Simulate how the CPU works. */
void cpu_exec(uint64_t n) {
  ...
  execute(n);
  ...
}

//可以看出每执行一条指令，device_update就会调用.
static void execute(uint64_t n) {
  Decode s;
  for (;n > 0; n --) {
    exec_once(&s, cpu.pc);
    ...
    IFDEF(CONFIG_DEVICE, device_update());
  }
}

/*
 * device_update()函数, 这个函数首先会检查距离上次设备更新是否已经超过一定时间,
 * 若是, 则会尝试刷新屏幕, 并进一步检查是否有按键按下/释放, 以及是否点击了窗口的X按钮; 
 * 否则则直接返回, 避免检查过于频繁
 */
void device_update() {
  static uint64_t last = 0;
  uint64_t now = get_time();
  if (now - last < 1000000 / TIMER_HZ) {
    return;
  }
  last = now;

  IFDEF(CONFIG_HAS_VGA, vga_update_screen());

#ifndef CONFIG_TARGET_AM
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_QUIT:
        nemu_state.state = NEMU_QUIT;
        break;
#ifdef CONFIG_HAS_KEYBOARD
      // If a key was pressed
      case SDL_KEYDOWN:
      case SDL_KEYUP: {
        uint8_t k = event.key.keysym.scancode;
        bool is_keydown = (event.key.type == SDL_KEYDOWN);
        send_key(k, is_keydown);
        break;
      }
#endif
      default: break;
    }
  }
#endif
}
```

> 我们先不管SDL_Event event 和 SDL_PollEvent(&event)，它们和SDL库有关
>
> 总之device_update的结果就是刷新了屏幕，让被按下的键盘码入键盘缓冲栈，判断NEMU是否退出。



## 运行时环境是什么？

运行时环境最终是要被编译链接成可执行文件，运行在NEMU硬件中的。

我们编写的运行时环境代码最底层均是内联汇编代码/汇编代码。

读内存我们通过直接访问某指定地址：

```c
//abstract-machine/am/src/riscv/riscv.h
static inline uint32_t inl(uintptr_t addr) { return *(volatile uint32_t *)addr; }
```

写内存我们通过直接访问某指定地址将数据写入：

```c
//abstract-machine/am/src/riscv/riscv.h
static inline void outl(uintptr_t addr, uint32_t data) { *(volatile uint32_t *)addr = data; }
```

总的来说，运行时环境是为了支持更复杂的程序运行，抽象对硬件访问方法，向程序提供更方便的功能。

# PA3 - 穿越时空的旅程: 批处理系统

- task PA3.1: 实现自陷操作`yield()`
- task PA3.2: 实现用户程序的加载和系统调用, 支撑TRM程序的运行
- task PA3.3: 运行仙剑奇侠传并展示批处理系统, 提交完整的实验报告

在PA中使用的操作系统叫Nanos-lite, 它是南京大学操作系统Nanos的裁剪版. 是一个为PA量身订造的操作系统. 通过编写Nanos-lite的代码, 你将会**认识到操作系统是如何使用硬件提供的机制(也就是ISA和AM)**, 来支撑程序的运行, 这也符合PA的终极目标. **软(AM, Nanos-lite)硬(NEMU)件**

Nanos-lite是运行在AM之上, AM的API在Nanos-lite中都是可用的. **虽然操作系统对我们来说是一个特殊的概念, 但在AM看来, 它只是一个调用AM API的普通C程序而已**, 和超级玛丽没什么区别. 

同时, 你会再次体会到AM的好处: **Nanos-lite的实现可以是架构无关的**, 这意味着, 无论你之前选择的是哪一款ISA, 都可以很容易地运行Nanos-lite.

**Nanos-lite本质上也是一个AM程序**, 我们可以**采用相同的方式来编译/运行Nanos-lite**.

## 最简单的操作系统

批处理系统的关键, 就是要有一个后台程序, 当一个前台程序执行结束的时候, 后台程序就会自动加载一个新的前台程序来执行.这样的一个后台程序, 其实就是操作系统.

具体又需要实现以下两点功能:

- 用户程序执行结束之后, 可以跳转到操作系统的代码继续执行
- 操作系统可以加载一个新的用户程序来执行

上述两点功能中其实蕴含着一个新的需求: 程序之间的执行流切换. 我们都不希望它可以把执行流切换到操作系统中的任意函数. 我们所希望的, 是一种可以限制入口的执行流切换方式.



>  **硬件需要提供一种可以限制入口的执行流切换方式. 这种方式就是自陷指令**，程序执行自陷指令之后, 就会陷入到操作系统预先设置好的跳转目标. 这个跳转目标也称为异常入口地址.
>
>  这一过程是ISA规范的一部分, 称为中断/异常响应机制.  称为中断/异常响应机制. 大部分ISA并不区分CPU的异常和自陷。目前我们并未加入硬件中断, 因此先把这个机制简称为"异常响应机制"吧.



riscv32提供**`ecall`指令作为自陷指令**, 并提供一个mtvec寄存器来存放异常入口地址. 为了保存程序当前的状态, riscv32提供了一些特殊的系统寄存器, 叫控制状态寄存器(CSR寄存器). 在PA中, 我们只使用如下4个CSR寄存器:

- mtvec寄存器 - 存放异常入口地址
- mepc寄存器 - 存放触发异常的PC
- mstatus寄存器 - 存放处理器的状态
- mcause寄存器 - 存放触发异常的原因

riscv32触发异常后硬件的响应过程如下:

1. 将当前PC值保存到mepc寄存器
2. 在mcause寄存器中设置异常号
3. 从mtvec寄存器中取出异常入口地址
4. 跳转到异常入口地址
5. riscv32通过`mret`指令从异常处理过程中返回, 它将根据mepc寄存器恢复PC.

**上述保存程序状态以及跳转到异常入口地址的工作, 都是硬件自动完成的, 不需要程序员编写指令来完成相应的内容(即这是我们再NEMU中要操作的内容)**

riscv32通过**`mret`指令从异常处理过程中返回**, 它将根据mepc寄存器恢复PC.



## 异常处理程序过程

### 设置异常入口地址

Nanos-lite会进行一项初始化工作: 调用`init_irq()`函数， 这最终会调用位于`abstract-machine/am/src/$ISA/nemu/cte.c`中的`cte_init()`函数. 

```c
void init_irq(void) {
  Log("Initializing interrupt/exception handler...");
  cte_init(do_event);
}
```

`cte_init()`函数会做两件事情,：

* 第一件就是设置异常入口地址
* 第二件事是注册一个事件处理回调函数, 这个回调函数由Nanos-lite提供.

```c
bool cte_init(Context*(*handler)(Event, Context*)) {
  // initialize exception entry
  asm volatile("csrw mtvec, %0" : : "r"(__am_asm_trap)); //设置异常入口地址，说明异常处理程序就是__am_asm_trap了

  // register event handler
  user_handler = handler; //注册Nanos-lite提供的事件处理回调函数

  return true;
}
```



### 触发自陷操作

Nanos-lite会在`panic()`前调用位于`/abstract-machine/am/src/riscv/nemu/cte.c`中的 `yield()`来触发自陷操作。

```c
void yield() {
#ifdef __riscv_e
  asm volatile("li a5, -1; ecall");
#else
  asm volatile("li a7, -1; ecall");
#endif
}
```

`yield`中的`ecall`在NEMU中实现，其会调用`isa_raise_intr`函数

NEMU中`isa_raise_intr()`函数 (在`nemu/src/isa/$ISA/system/intr.c`中定义)模拟了上文提到的异常响应机制，支撑了自陷操作。

```c
word_t isa_raise_intr(word_t NO, vaddr_t epc) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * Then return the address of the interrupt/exception vector.
   */
  cpu.csrs.mcause = NO;
  cpu.csrs.mepc = epc;
  return cpu.csrs.mtvec;
}
```

NEMU在指令解析并执行时会调用`isa_raise_intr`，这个函数干的事情就是设置异常号，异常发生时PC值，返回异常入口地址。



### 保存上下文

但通常硬件并不负责保存它们, 因此需要通过**软件代码来保存它们的值**.

riscv32则通过`sw`指令将各个通用寄存器依次压栈.

保存上下文的操作在异常入口地址对应的函数__am_asm_trap完成了，部分代码如下：

`/abstract-machine/am/src/riscv/nemu/trap.S`

```c
#if __riscv_xlen == 32
#define LOAD  lw
#define STORE sw
#define XLEN  4
#else
#define LOAD  ld
#define STORE sd
#define XLEN  8
#endif

.align 3
.globl __am_asm_trap
__am_asm_trap:
  //保存上下文进栈
  addi sp, sp, -CONTEXT_SIZE

  MAP(REGS, PUSH)

  csrr t0, mcause
  csrr t1, mstatus
  csrr t2, mepc

  STORE t0, OFFSET_CAUSE(sp)
  STORE t1, OFFSET_STATUS(sp)
  STORE t2, OFFSET_EPC(sp)

  # set mstatus.MPRV to pass difftest
  li a0, (1 << 17)
  or t1, t1, a0
  csrw mstatus, t1

  mv a0, sp
  jal __am_irq_handle //跳转到事件处理函数处
  //恢复上下文
  LOAD t1, OFFSET_STATUS(sp)
  LOAD t2, OFFSET_EPC(sp)
  csrw mstatus, t1
  csrw mepc, t2

  MAP(REGS, POP)

  addi sp, sp, CONTEXT_SIZE
  mret
```

### 事件分发

`__am_irq_handle()`的代码会把执行流切换的原因打包成事件, 然后调用在`cte_init()`中注册的事件处理回调函数`do_event()`函数(`nanos-lite/src/irq.c`中的), 将事件交给Nanos-lite来处理. 

```c
Context* __am_irq_handle(Context *c) {
  if (user_handler) {
    Event ev = {0};
    // debugContext(c);
    switch (c->mcause) {
      case (uintptr_t)-1: ev.event = EVENT_YIELD;   break; 
      case (uintptr_t) 0:
      case (uintptr_t) 1: 
      case (uintptr_t) 2: 
      case (uintptr_t) 3: 
      case (uintptr_t) 4:
      case (uintptr_t) 7:
      case (uintptr_t) 8:
      case (uintptr_t) 9: 
      case (uintptr_t) 13: 
      case (uintptr_t) 19: ev.event = EVENT_SYSCALL; break;
      default: assert(0); ev.event = EVENT_ERROR; break;
    }

    c = user_handler(ev, c); //user_handle是Nanos-lite注册的异常处理函数
    assert(c != NULL);
  }
  c->mepc = c->mepc + 4;
  return c;
}
```



 `do_event()`函数会根据事件类型再次进行分发.

```c
static Context* do_event(Event e, Context* c) {
  switch (e.event) {
    case EVENT_YIELD:
      Log("Nanos in yield"); break;
    case EVENT_SYSCALL:
      do_syscall(c); break;
    default: panic("Unhandled event ID = %d", e.event);
  }
  return c;
}

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;
  //printf("do_syscall: %d\n", (int)a[0]);
  switch (a[0]) {
    case (uintptr_t) 0: sys_exit(c);  break;
    case (uintptr_t) 1: sys_yield(c); break;
    case (uintptr_t) 2: sys_open(c);  break;
    case (uintptr_t) 3: sys_read(c);  break;
    case (uintptr_t) 4: sys_write(c); break;
    case (uintptr_t) 7: sys_close(c); break;
    case (uintptr_t) 8: sys_lseek(c); break;
    case (uintptr_t) 9: sys_brk(c);   break;
    case (uintptr_t) 13: sys_execve(c);       break;
    case (uintptr_t) 19: sys_gettimeofday(c); break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
}
```

### 恢复上下文

很简单，实现在__am_asm_trap中，就是将压入栈的数据恢复



## Nanos-lite操作系统

一般来说, 程序应该存放在永久存储的介质中(比如磁盘). 但要在NEMU中对磁盘进行模拟是一个略显复杂工作, 因此先**让Nanos-lite把其中的一段内存作为磁盘来使用. 这样的磁盘有一个专门的名字, 叫ramdisk.**

用户程序运行在操作系统之上, 由于运行时环境的差异, 我们不能把编译到AM上的程序放到操作系统上运行. 为此, 我们准备了一个新的子项目Navy-apps, 专门用于编译出操作系统的用户程序. 

Navy负责编译Nanos-lite和运行在Nanos-lite上的用户程序，具体操作为：

> 用户程序在navy-app下，通过`make ISA=$ISA`将用户程序生成为Nanos-lite的可执行文件, 编译期间会把ramdisk镜像文件`nanos-lite/build/ramdisk.img` 包含进Nanos-lite成为其中的一部分(在`nanos-lite/src/resources.S`中实现). 
>
> 为了避免和Nanos-lite的内容产生冲突, 我们约定目前用户程序需要被链接到内存位置`0x83000000`（riscv32)附近, Navy已经设置好了相应的选项(见`navy-apps/scripts/$ISA.mk`中的`LDFLAGS`变量). 

因为Nanos-lite无非就是调用AM的API的C程序，所以我们使用在AM中配置好的Makefile等进行[交叉编译](http://en.wikipedia.org/wiki/Cross_compiler)，生成可以在NEMU中识别并运行的可执行文件

在`/abstract-machine/scripts/linker.ld`中设定了链接时段的分布

```c
ENTRY(_start)
PHDRS { text PT_LOAD; data PT_LOAD; }

SECTIONS {
  /* _pmem_start and _entry_offset are defined in LDFLAGS */
  . = _pmem_start + _entry_offset;
  .text : {
    *(entry)
    *(.text*)
  } : text
  etext = .;
  _etext = .;
  .rodata : {
    *(.rodata*)
  }
  .data : {
    *(.data)
  } : data
  edata = .;
  _data = .;
  .bss : {
	_bss_start = .;
    *(.bss*)
    *(.sbss*)
    *(.scommon)
  }
  _stack_top = ALIGN(0x1000);
  . = _stack_top + 0x8000;
  _stack_pointer = .;
  end = .;
  _end = .;
  _heap_start = ALIGN(0x1000);
}
```

可以看到堆和栈出现了！那么在NEMU中内存映像为：

![image](https://img2024.cnblogs.com/blog/2641775/202404/2641775-20240423195810063-459472770.png)

### 加载器

作用：在操作系统中, 加载用户程序是由loader(加载器)模块负责的. 

我们知道程序中包括代码和数据, 它们都是存储在可执行文件中. 

加载的过程就是把可执行文件中的代码和数据放置在正确的内存位置, 然后跳转到程序入口, 程序就开始执行了. 

* 可执行文件在哪里? 

  可执行文件位于ramdisk偏移为0处, 访问它就可以得到用户程序的第一个字节.

* 代码和数据在可执行文件的哪个位置?

  通过解析ELF文件格式得到：它除了包含程序本身的代码和静态数据之外, 还包括一些用来描述它们的信息,这些信息描述了可执行文件的组织形式

根据上述内容，我们知道最终Nanos-lite和用户程序都会成为可执行文件被放到NEMU的‘内存’中，所以加载器最终是要通过访问NEMU的内存实现加载用户程序的，那么访问NEMU的API为：

```c
// 从ramdisk中`offset`偏移处的`len`字节读入到`buf`中
size_t ramdisk_read(void *buf, size_t offset, size_t len);

// 把`buf`中的`len`字节写入到ramdisk中`offset`偏移处
size_t ramdisk_write(const void *buf, size_t offset, size_t len);

// 返回ramdisk的大小, 单位为字节
size_t get_ramdisk_size();
```

loader的工作向我们展现出了程序的最为原始的状态: 比特串! 加载程序其实就是把这一毫不起眼的比特串放置在正确的位置

（ 加载一个可执行文件并不是加载它所包含的所有内容, 只要加载那些与运行时刻相关的内容就可以了, 例如调试信息和符号表就不必加载. 我们可以通过判断segment的`Type`属性是否为`PT_LOAD`来判断一个segment是否需要加载.）

### 堆区管理

堆区的使用情况是由libc来进行管理的, 但堆区的大小却需要通过系统调用向操作系统提出更改.

调整堆区大小是通过`sbrk()`库函数来实现的，在Navy的Newlib中, `sbrk()`最终会调用`_sbrk()`

事实上, 用户程序在第一次调用`printf()`的时候会尝试通过`malloc()`申请一片缓冲区, 来存放格式化的内容.

为了实现`_sbrk()`的功能, 我们还需要提供一个用于设置堆区大小的系统调用. 在GNU/Linux中, 这个系统调用是`SYS_brk`, 它接收一个参数`addr`, 用于指示新的program break的位置.

目前Nanos-lite还是一个单任务操作系统, 空闲的内存都可以让用户程序自由使用, 因此我们只需要让`SYS_brk`系统调用总是返回`0`即可, 表示堆区大小的调整总是成功. 

### 简易文件系统

要实现一个完整的批处理系统, 我们还需要向系统提供多个程序. 

**我们之前把程序以文件的形式存放在ramdisk之中, 但如果程序的数量增加之后, 我们就要知道哪个程序在ramdisk的什么位置.**

我们对文件系统的需求并不是那么复杂, 因此我们可以定义一个简易文件系统sfs(Simple File System):

- 每个文件的大小是固定的
- 写文件时不允许超过原有文件的大小
- 文件的数量是固定的, 不能创建新文件
- 没有目录

既然文件的数量和大小都是固定的, 我们自然可以把每一个文件分别固定在ramdisk中的某一个位置。我们约定文件从ramdisk的最开始一个挨着一个地存放:

```
0
+-------------+---------+----------+-----------+--
|    file0    |  file1  |  ......  |   filen   |
+-------------+---------+----------+-----------+--
 \           / \       /            \         /
  +  size0  +   +size1+              + sizen +
```

为了记录ramdisk中各个文件的名字和大小, 我们还需要一张"文件记录表".

**"文件记录表"其实是一个数组**, 数组的每个元素都是一个结构体:

```c
typedef struct {
  char *name;         // 文件名
  size_t size;        // 文件大小
  size_t disk_offset;  // 文件在ramdisk中的偏移
  ReadFn read;        // 读函数指针
  WriteFn write;      // 写函数指针
} Finfo;
```

在sfs中, 这三项信息都是固定不变的. 其中的文件名和我们平常使用的习惯不太一样: 由于sfs没有目录, 我们把目录分隔符`/`也认为是文件名的一部分

实际上, 操作系统中确实存在不少"没有名字"的文件(如标准输入输出和标准错误输出). 为了统一管理它们, 我们希望通过一个编号来表示文件, 这个编号就是**文件描述符(file descriptor)**. 一个文件描述符对应一个正在打开的文件, 由操作系统来维护文件描述符到具体文件的映射. 

在Nanos-lite中, 由于sfs的文件数目是固定的, **我们可以简单地把文件记录表的下标作为相应文件的文件描述符返回给用户程序. **

###  一切皆文件，把IOE抽象成文件

AM中的IOE向我们展现了程序进行输入输出的需求. 那么在Nanos-lite上, 如果用户程序想访问设备, 要怎么办呢?

> - 设备的类型五花八门, 其功能更是数不胜数, 要为它们分别实现系统调用来给用户程序提供接口, 本身就已经缺乏可行性了;
> - 此外, 由于设备的功能差别较大, 若提供的接口不能统一, 程序和设备之间的交互就会变得困难. 所以我们需要有一种方式对设备的功能进行抽象, 向用户程序提供统一的接口.

文件就是字节序列, 那很自然地, 上面这些五花八门的字节序列应该都可以看成文件. 

Unix就是这样做的, 因此有"一切皆文件"(Everything is a file)的说法. 这种做法最直观的好处就是为不同的事物提供了统一的接口: 我们可以使用文件的接口来操作计算机上的一切, 而不必对它们进行详细的区分: 

> 我们对之前实现的文件操作API的语义进行扩展, 让它们可以支持任意文件(包括"特殊文件")的操作:
>
> 这组扩展语义之后的API有一个酷炫的名字, 叫[VFS(虚拟文件系统)](https://en.wikipedia.org/wiki/Virtual_file_system). 既然有虚拟文件系统, 那相应地也应该有"真实文件系统", 这里所谓的真实文件系统, 其实是指具体如何操作某一类文件.
>
> > 在真实的操作系统上, 真实文件系统的种类更是数不胜数: 比如熟悉Windows的你应该知道管理普通文件的NTFS, 目前在GNU/Linux上比较流行的则是EXT4; 至于特殊文件的种类就更多了
>
> 所以, VFS其实是对不同种类的真实文件系统的抽象, 它用一组API来描述了这些真实文件系统的抽象行为, 屏蔽了真实文件系统之间的差异, 

在Nanos-lite中, 实现VFS的关键就是`Finfo`结构体中的两个读写函数指针:

其中`ReadFn`和`WriteFn`分别是两种函数指针, 它们用于指向真正进行读写的函数, 并返回成功读写的字节数. 有了这两个函数指针, 我们只需要在文件记录表中对不同的文件设置不同的读写函数, 就可以通过`f->read()`和`f->write()`的方式来调用具体的读写函数了.

```c
static Finfo file_table[] __attribute__((used)) = {
  [FD_STDIN]  = {"stdin", 0, 0, invalid_read, invalid_write},
  [FD_STDOUT] = {"stdout", 0, 0, invalid_read, serial_write},
  [FD_STDERR] = {"stderr", 0, 0, invalid_read, serial_write},
  [FD_EVENT]  = {"/dev/events", 0, 0, events_read, invalid_write},
  [FD_FB]     = {"/dev/fb", 0, 0, invalid_read, fb_write}, 
  [FD_DISPINFO] = {"/proc/dispinfo", 0, 0, dispinfo_read, invalid_write},
#include "files.h"
};
```

为了更好地封装IOE的功能, 我们在Navy中提供了一个叫NDL(NJU DirectMedia Layer)的多媒体库. 这个库的代码位于`navy-apps/libs/libndl/NDL.c`中。

NDL…嗯我们已经实现了自己的SDL库了



# 运行方式

从仓库clone之后

```
cd nemu/
make menuconfig
# 然后选择开启device, 关闭debugger, 保存得到.config
cd nanos-lite/
make ARCH=$ISA-nemu update //得到ramdisk.img
make ARCH=$ISA-nemu run
# 可能有些环境变量如$ISA=riscv32， 要设置下
```

然后回出现开机界面，任意按下按钮后到终端

终端输入`help`后会有程序名字出现，输入名字执行相应程序

# 总结

首先我想要解释下NEMU项目的架构：我将NEMU项目的架构分为4层：

* 最下层为模拟的硬件。
* 中下层为ISA接口，提供软硬件协作的规范
* 中上层为运行时环境，抽象对硬件的操作方法，向程序提供更方便的功能。
* 最上层为操作系统，操作系统本质上是调用运行时环境API的C语言程序。

最终运行时环境和操作系统都被编写链接成可执行文件在硬件中运行。

> * 做了什么
> * 遇到了什么困难
> * 如何解决的

基于 Github 开源教学项目 NJU EMUlator，探究"程序在计算机上运行"的基本原理。

在项目中，我查阅 RISCV32 的 ISA 手册，使用数据对象模拟出通用寄存器，控制状态寄存器， 内存等硬件，以及取指，译码，执行，读写内存，更新 PC 的计算机运行过程，实现了基于冯诺依曼体系结构的“客户计算机”。

总的来说我们模拟了一个硬件的世界, 要做的事情是编写一个用来执行其它程序的程序! 

<hr>


> 困难：调试。
>
> 每当NEMU出现奇怪的Bug时，用外部调试器如GDB总是需要花费较多时间进行调试，因为若将NEMU作为一个状态机，NEMU每次状态发生变化是因为NEMU模拟执行了一条指令。
>
> 同时NEMU是一个用来执行其它客户程序的程序, 这意味着, NEMU可以随时了解客户程序执行的所有信息. 然而这些信息对外面的调试器(例如GDB)来说, 是不容易获取的. 
>
> 例如在通过GDB调试NEMU的时候, 你将很难在NEMU中运行的客户程序中设置断点, 但对于NEMU来说, 这是一件不太困难的事情.

解决方法：

实现简易调试器（类似 gdb）——能够解析并执行用户输 入的命令；打印客户计算机的状态（包括 通用寄存器，内存等中的值）；同时支 持表达式求值，设置监视点和断点，便于调试客户计算机。 

> 实现简易调试器的方法：
>
> 我们将取指，译码，执行，读写内存，更新PC的操作封装成函数execue_one()，简易调试器做的就是通过设置宏决定进入批处理模式还是调试模式，若进入调试模式，则不断从用户得到输入的命令，并解析，并调用对应的函数。
>
> 设置监视点和断点的方法：
>
> 监视点：实现监视点的前提是实现表达式求值，监视点就是不断查看表达式是否为true，若为true则打印对应信息。
>
> 断点的实现方式则是利用监视点，将表达式写为 pc \== 要停下来的指令地址。
>
> 当 pc \== 要停下来的指令地址，则停止运行，输出信息、

<hr>


基于SDL库，实现模拟设备：

SDL库在现实世界读取来自键盘等的信息，或输出程序产生的图像/音频数据。并每次在cpu执行完一条指令后，更新设备状态。

基于内存映射I/O思想，将设备寄存器空间映射到NEMU内存空间上，这样我们可以复用访问NEMU内存的API，像访问内存一样访问设备。

* 键盘

  在硬件层面，键盘我用循环队列做缓存，保存来自外界的键盘输出信号。注册键盘读回调函数，当访问键盘寄存器时，先触发回调函数，从循环队列中pop出数据存放到键盘寄存器(即这是个准备数据的过程)。

  在ISA层面，NEMU识别出访存指令后，调用vaddr_read，最终调用map_read来访问键盘寄存器。

* 串口

  串口是用于实现界面输出的（类似实现printf的效果）

  在硬件层面，串口最终使用putch进行输出

<hr>


为使客户计算机能够运行更加复杂的程序，基于abstract-machine项目，为客户计算机提供更复杂的运行时环境，我所实现的包括输入输出扩展，上下文管理，堆管理，常用库函数。


* 输入输出扩展，通过抽象将访问设备的方式统一为两个API：ioe_write, ioe_read 。具体操作是各个设备实现各自的读写方法，然后通过宏与函数指针，ioe_write, ioe_read做的只是分发。
* 上下文管理，用数据结构模拟出保存到栈的上下文信息结构体，实现cte_init(操作系统调用此API：设置异常入口地址，注册事件处理函)，实现异常处理程序，实现自陷操作。
* 堆管理，实现系统调用sys_brk()进行管理。
* 常用库函数，包括memset，strcpy等函数。

<hr>


基于 Nanos-lite 开源项目，编写能够进行批处理的操作系统。

我主要做了三件事：

* 实现异常处理

  * 实现异常处理需要操作系统协作，以及硬件的支持。具体实现时，我通过编写内联汇编或汇编代码模拟硬件支持。
  * 操作系统在初始化时设置异常处理入口地址，需要硬件支持，用内联汇编将异常处理程序的地址保存到异常入口寄存器。同时注册事件处理函数
  * 用汇编代码编写异常处理程序，包括保存上下文，将异常封装成事件（为了扩展，因为异常包括中断，系统调用，故障，错误），调用事件处理函数，恢复上下文。
  * 实现自陷操作，需要硬件支持，用内联汇编调用指令ecall，NEMU识别指令并获取指令中的异常号保存到异常号寄存器，保存当前pc到异常pc寄存器, 将pc设置成异常处理入口地址。

* 实现简易文件系统

  * 首先实现了简易磁盘，即将NEMU的一块连续的内存区域作为磁盘使用。

  * 实现简易文件系统（每个文件的大小，位置是固定的, 写文件时不允许超过原有文件的大小, 文件的数量是固定的, 不能创建新文件, 没有目录），即文件的名称，数量，大小，在磁盘中的位置都是提前设置的。

    文件系统在磁盘上，我们维护"文件记录表"这个数据结构，他记录了文件名，大小，磁盘中偏移量，读写函数指针

  * 目的：

    * 向系统提供多个程序.
    * 为不同的事物提供了统一的接口: 我们可以使用文件的接口来操作计算机上的一切, 而不必对它们进行详细的区分。（一切皆文件）

* 实现加载器

  * 因为已经实现了简易文件系统，加载器就是读保存在文件系统中的可执行文件。通过解析ELF格式，将代码和数据加载进NEMU的内存中，更新PC。
