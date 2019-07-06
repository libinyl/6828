# 初始化实验环境

1. 直接执行 [initlab.sh](initlab.sh), 下载实验所需代码
2. 初始化 `QEMU`:

    配置 qemu

    ```shell
    cd qemu
    ./configure --disable-kvm --disable-werror  --target-list="i386-softmmu x86_64-softmmu"
    ```

    如果配置 qemu 时缺少包（在 initlab.sh 中应已执行）:

    ```shell
    apt install pkg-config libglib2.0-dev zlib1g-dev libpixman-1-dev
    ```

    编译安装 qemu
    ```shell
    make && make install
    ```
# [Lab 1: Booting a PC](https://pdos.csail.mit.edu/6.828/2018/labs/lab1/)

## Part 1: PC Bootstrap

1. 编译内核
    ```shell
    cd lab
    make
    ```

    生成了文件`obj/kern/kernel.img`, 就是我们的虚拟**硬盘**. 这个"硬盘"镜像中包含了 **boot loader** (obj/boot/boot) 和内核 **obj/kernel** . 
    
    生成的目录结构：
    ```
    root@MyServer:~/6828/lab# tree obj
    obj
    |-- boot
    |   |-- boot
    |   |-- boot.asm
    |   |-- boot.o
    |   |-- boot.out
    |   `-- main.o
    `-- kern
        |-- console.o
        |-- entry.o
        |-- entrypgdir.o
        |-- init.o
        |-- kdebug.o
        |-- kernel
        |-- kernel.asm
        |-- kernel.img
        |-- kernel.sym
        |-- monitor.o
        |-- printf.o
        |-- printfmt.o
        |-- readline.o
        `-- string.o
    ```

    下一步，启动 QEMU, 加载硬盘。

2. 启动 QEMU
    ```shell
    cd lab
    make qemu-nox
    ```

    此命令启动 QEMU, 将输出

    ```shell
    root@MyServer:~/6828/lab#     make qemu-nox
    sed "s/localhost:1234/localhost:25000/" < .gdbinit.tmpl > .gdbinit
    ***
    *** Use Ctrl-a x to exit qemu
    ***
    qemu-system-i386 -nographic -drive file=obj/kern/kernel.img,index=0,media=disk,format=raw -serial mon:stdio -gdb tcp::25000 -D qemu.log
    6828 decimal is XXX octal!
    entering test_backtrace 5
    entering test_backtrace 4
    entering test_backtrace 3
    entering test_backtrace 2
    entering test_backtrace 1
    entering test_backtrace 0
    leaving test_backtrace 0
    leaving test_backtrace 1
    leaving test_backtrace 2
    leaving test_backtrace 3
    leaving test_backtrace 4
    leaving test_backtrace 5
    Welcome to the JOS kernel monitor!
    Type 'help' for a list of commands.
    K>
    ```
    退出方式：`Ctrl+a`（松手）`x`.

    此时只有两个有效命令：`help`和`kerninfo`. 后者的输出是：

    ```shell
    K> kerninfo
    Special kernel symbols:
    _start                  0010000c (phys)
    entry  f010000c (virt)  0010000c (phys)
    etext  f0101871 (virt)  00101871 (phys)
    edata  f0112300 (virt)  00112300 (phys)
    end    f0112940 (virt)  00112940 (phys)
    Kernel executable memory footprint: 75KB
    ```

3. 调试内核
   
    首先退出 qemu 执行环境，执行`make qemu-gdb`进入 qemu 调试环境；
    
    ```shell
    root@MyServer:~/6828/lab# make qemu-gdb
    ***
    *** Now run 'make gdb'.
    ***
    qemu-system-i386 -drive file=obj/kern/kernel.img,index=0,media=disk,format=raw -serial mon:stdio -gdb tcp::25000 -D qemu.log  -S
    VNC server running on `::1:5900'
    ```
    
    然后在另一 session 用 gdb 链接此环境，只需执行调试内核命令`make gdb`:

   ```shell
    root@MyServer:~/6828/lab# make gdb
    gdb -n -x .gdbinit
    ...（省略一些信息）...
    + target remote localhost:25000
    warning: A handler for the OS ABI "GNU/Linux" is not built into this configuration
    of GDB.  Attempting to continue with the default i8086 settings.

    The target architecture is assumed to be i8086
    [f000:fff0]    0xffff0:	ljmp   $0xf000,$0xe05b
    0x0000fff0 in ?? ()
    + symbol-file obj/kern/kernel
    (gdb)
    ```

    我们可以获取的信息有：

    - qemu 从物理地址`0xffff0`开始执行。`f000:fff0`表示`0x f000`*16+`0x fff0`=`0xf fff0`. 这对 qemu 来说是写死了的，第一条指令必须从这里开始执行。
    - PC 启动时，位于`CS = 0xf000` , `IP = 0xfff0`. 的**段地址**位置。*段地址=CS\*16+IP*.
        ```shell
        16 * 0xf000 + 0xfff0   # in hex multiplication by 16 is
        = 0xf0000 + 0xfff0     # easy--just append a 0.
        = 0xffff0 
        ```
    - 执行的第一条指令是`jmp`, 跳转到`CS = 0xf000` ,`IP = 0xe05b`的地址。
  
    结合物理内存分布来看：
    ```
    +------------------+  <- 0xFFFFFFFF (4GB)
    |      32-bit      |
    |  memory mapped   |
    |     devices      |
    |                  |
    /\/\/\/\/\/\/\/\/\/\

    /\/\/\/\/\/\/\/\/\/\
    |                  |
    |      Unused      |
    |                  |
    +------------------+  <- depends on amount of RAM
    |                  |
    | Extended Memory  |
    |                  |
    +------------------+  <- 0x00100000 (1MB)
    |     BIOS ROM     |
    +------------------+  <- 0x000F0000 (960KB)
    |  16-bit devices, |
    |  expansion ROMs  |
    +------------------+  <- 0x000C0000 (768KB)
    |   VGA Display    |
    +------------------+  <- 0x000A0000 (640KB)
    |                  |
    |    Low Memory    |
    |                  |
    +------------------+  <- 0x00000000
    ```
    第一条指令所在的地址`0xffff0`距离 BIOS 结束 (`0x100000`) 只剩 16 个 byte , 啥都做不了，所以第一条指令是跳转到了 BIOS 中更早一些的位置。

    在 BIOS 初始化了重要的设备 (device) 之后，BIOS 开始读取硬盘上的 boot loader, 并将控制权转移给它。

## Part 2: The Boot Loader

引导**硬盘/软盘**的最小读写单位是**扇区** (sector), 每块 512 个 byte. 每次读写操作必须是整数个 sector 单位。

1. 读取引导扇区的 512 个 byte 至物理地址从`0x7c00`始到`0x7dff`正好 512 个 byte 的位置。
2. 执行 `jmp` 指令设置 CS:IP 为`0000:7c00`, 也就是引导扇区内容所在的物理内存，至此将控制权移交给 boot loader. *这几个地址都是神器的魔数，无需深究。但是这是业界通用的。*

引导**光驱**的步骤更复杂，也更强大。扇区单位变成了 2018 个 byte, 且 BIOS 会读取多个扇区。

对于本实验而言，使用的是传统的 512 字节读取的方式。boot loader 包含一个汇编源文件：`boot/boot.S`和一个 C 源文件：`boot/main.c`. 它们主要执行两个任务：

- 首先，boot loader 将处理器从 16 位的 real mode 转换到 32 位的保护模式。只有在保护模式，软件才能访问物理内存中 1MB 以上的部分。
- 然后，boot loader 从通过 x86 专用 IO 指令的方式硬盘读取内核代码。

**理解源代码**

`boot.S`的工作：

1. 被 BIOS 从硬盘上的第一个扇区读取到`0x7c00`, 并在实模式下执行，执行开始时 cs=0 ,ip=7c00.
2. 通过`lgdt gdtdesc`,` orl $CR0_PE_ON, %eax`等指令切换至实模式；
3. 跳转到 C 代码并执行，
    ```
    movl    $start, %esp
    call bootmain
    ```
`boot/main.c`的工作：

加载内核镜像。如何加载？内核镜像是 ELF 二进制文件格式。我们对三个**段 (section) **感兴趣：

- `.text`:  程序的可执行指令
- `.rodata`: 只读数据，如 C 编译器生成的字符串常量
- `.data`: 程序的初始化数据，例如 `int x = 5` 声明的全局变量。

> 当连接器计算程序内存布局时，会在紧邻`.data`段的`bss`段为 *未初始化的全局变量* 保留空间，如 `int x;`.C 语言要求未初始化的全局变量值为 0. 因此不需要在 ELF 二进制文件中保存 `bss` 的内容。链接器只记录`.bss`段的地址和大小。程序加载器或者程序本身必须将。`bss` 段归零。

**练习三：**
1. 处理器何时开始执行 32 位代码？什么最终导致了 16 位模式到 32 位模式的切换？
   
   答：boot.S 48~51 行。将 `cr0` 寄存器的最后一位置打开，即开启了保护模式。

    ```
    lgdt    gdtdesc
    movl    %cr0, %eax
    orl     $CR0_PE_ON, %eax
    movl    %eax, %cr0
    ```
    > lgdt : 加载全局中断描述符
    > orl : 逻辑或

2. boot loader 执行的最后一个指令是什么，它读取的内核的第一行代码是什么？
   
   答：最后一行代码是`((void (*)(void)) (ELFHDR->e_entry))();`, 位于`main.c`的第 60 行。

   内核的入口可以通过以下命令查看：

    ```
    root@MyServer:~/6828/lab# objdump -f obj/kern/kernel

    obj/kern/kernel:     file format elf32-i386
    architecture: i386, flags 0x00000112:
    EXEC_P, HAS_SYMS, D_PAGED
    start address 0x0010000c       <----------  entry point
    ```

    如`entry.S`所说，`_start`指定了 ELF 文件的入口。故内核的入口位于`entry.S`的第 44 行`movw	$0x1234,0x472			# warm boot`.

3. 内核的第一个指令在哪？

    内核是一个 elf 文件，它本身描述了 loader 的进程从哪里开始执行。通过`readelf -h`可以看到 elf 文件头描述的入口点：

    ```
    root@MyServer:~/6828/lab/obj/kern# readelf -h kernel
    ELF Header:
    Magic:   7f 45 4c 46 01 01 01 00 00 00 00 00 00 00 00 00
    Class:                             ELF32
    Data:                              2's complement, little endian
    Version:                           1 (current)
    OS/ABI:                            UNIX - System V
    ABI Version:                       0
    Type:                              EXEC (Executable file)
    Machine:                           Intel 80386
    Version:                           0x1
    Entry point address:               0x10000c  <=========== 进程执行入口
    Start of program headers:          52 (bytes into file)
    Start of section headers:          82728 (bytes into file)
    Flags:                             0x0
    Size of this header:               52 (bytes)
    Size of program headers:           32 (bytes)
    Number of program headers:         3
    Size of section headers:           40 (bytes)
    Number of section headers:         11
    Section header string table index: 8
    ```

4. 为了从磁盘中读取整个 kernel,boot loader 是如何决定读取多少个扇区的？它是怎么找到这个信息的？
   
    ELF 头文件信息指明了最后的位置。

**如何查看内核 ELF 文件所有段的信息？**

```shell
cd obj/
objdump -h kern/kernel
```
输出：

```shell
root@MyServer:~/6828/lab/obj# objdump -h kern/kernel

kern/kernel:     file format elf32-i386

Sections:
Idx Name          Size      VMA       LMA       File off  Algn
  0 .text         00001871  f0100000  00100000  00001000  2**4
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
  1 .rodata       00000714  f0101880  00101880  00002880  2**5
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  2 .stab         000038d1  f0101f94  00101f94  00002f94  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  3 .stabstr      000018bb  f0105865  00105865  00006865  2**0
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  4 .data         0000a300  f0108000  00108000  00009000  2**12
                  CONTENTS, ALLOC, LOAD, DATA
  5 .bss          00000648  f0112300  00112300  00013300  2**5
                  CONTENTS, ALLOC, LOAD, DATA
  6 .comment      00000035  00000000  00000000  00013948  2**0
                  CONTENTS, READONLY
```
> VMA: Virtual Memory Address, 链接地址
> LMA: Load Memory Address, 加载地址

(*关于 VMA 和 LMA, 详细可参考这篇 [博客](https://blog.csdn.net/cinmyheart/article/details/39762595)。*)

- 段被加载到内存，然后执行。
- 加载地址是段希望被加载到的内存的地址，链接地址是段期望从哪里开始执行的地址。
- 这些地址与内容相关，只有在特定的地址执行才会有意义（个人理解）. 现代的个共享库也可以生成与地址无关的代码，但本实验不采用。
- 通常链接地址与加载地址相同。如：
    ```
    root@MyServer:~/6828/lab# objdump -h obj/boot/boot.out

    obj/boot/boot.out:     file format elf32-i386

    Sections:
    Idx Name          Size      VMA       LMA       File off  Algn
    0 .text         00000186  00007c00  00007c00  00000074  2**2
                    CONTENTS, ALLOC, LOAD, CODE
    1 .eh_frame     000000a8  00007d88  00007d88  000001fc  2**2
                    CONTENTS, ALLOC, LOAD, READONLY, DATA
    2 .stab         00000720  00000000  00000000  000002a4  2**2
                    CONTENTS, READONLY, DEBUGGING
    3 .stabstr      0000088f  00000000  00000000  000009c4  2**0
                    CONTENTS, READONLY, DEBUGGING
    4 .comment      00000035  00000000  00000000  00001253  2**0
                    CONTENTS, READONLY
    ```

- boot loader 通过 ELF 的 **header** 来决定如何加载段。如何查看 header?
    ```
    objdump -x obj/kern/kernel
    ```
    输出：
    ```
    ... 省略。..
    Program Header:
        LOAD off    0x00001000 vaddr 0xf0100000 paddr 0x00100000 align 2**12
             filesz 0x00007120 memsz 0x00007120 flags r-x
        LOAD off    0x00009000 vaddr 0xf0108000 paddr 0x00108000 align 2**12
             filesz 0x0000a948 memsz 0x0000a948 flags rw-
       STACK off    0x00000000 vaddr 0x00000000 paddr 0x00000000 align 2**4
             filesz 0x00000000 memsz 0x00000000 flags rwx
    ... 省略。..
    ```
    被标记为"load"的对象需要被加载到内存中。输出也给出了虚拟地址 (addr), 物理地址 (paddr), 加载区域大小 (memsz, filesz).

在`boot/main.c`中，`ph->p_pa`字段包含段的目标物理地址。

```C
// elf.h
struct Proghdr {
	uint32_t p_type;
	uint32_t p_offset;
	uint32_t p_va;
	uint32_t p_pa;
	uint32_t p_filesz;
	uint32_t p_memsz;
	uint32_t p_flags;
	uint32_t p_align;
};

//main.c
void
bootmain(void)
{
	struct Proghdr *ph, *eph;

	// read 1st page off disk
	readseg((uint32_t) ELFHDR, SECTSIZE*8, 0);

	...

	// 加载每个段到目标地址
	ph = (struct Proghdr *) ((uint8_t *) ELFHDR + ELFHDR->e_phoff);
	eph = ph + ELFHDR->e_phnum;
	for (; ph < eph; ph++)
		// p_pa 就是这个段的目标加载地址，也就是物理地址
		readseg(ph->p_pa, ph->p_memsz, ph->p_offset);

	// 从 ELF hdader 中调用 entry point, 且永不返回
	((void (*)(void)) (ELFHDR->e_entry))();
....
}
```

**entry point** 在哪？

```
objdump -f obj/kern/kernel
```

输出：

```
root@MyServer:~/6828/lab# objdump -f obj/kern/kernel

obj/kern/kernel:     file format elf32-i386
architecture: i386, flags 0x00000112:
EXEC_P, HAS_SYMS, D_PAGED
start address 0x0010000c       <----------  entry point
```

**总结** `boot/main.c`包含一个极简的 ELF 加载器，它把硬盘上 **kernel **的每个段都加载到内存中（的目标加载地址）, 然后跳转到内核的 **entry point**。

## Part 3: The Kernel

和 boot loader 类似，kernel 部分也是从几句汇编开始（用于初始化 C 语言环境）。

虽然 boot loader 的链接地址与加载地址相同，但 kernel 的链接地址和加载地址相差很大：

```shell
root@MyServer:~/6828/lab/obj# objdump -h kern/kernel

kern/kernel:     file format elf32-i386

Sections:
Idx Name          Size      VMA       LMA       File off  Algn
  0 .text         00001871  f0100000  00100000  00001000  2**4
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
  1 .rodata       00000714  f0101880  00101880  00002880  2**5
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  2 .stab         000038d1  f0101f94  00101f94  00002f94  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  3 .stabstr      000018bb  f0105865  00105865  00006865  2**0
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  4 .data         0000a300  f0108000  00108000  00009000  2**12
                  CONTENTS, ALLOC, LOAD, DATA
  5 .bss          00000648  f0112300  00112300  00013300  2**5
                  CONTENTS, ALLOC, LOAD, DATA
  6 .comment      00000035  00000000  00000000  00013948  2**0
                  CONTENTS, READONLY
```

链接内核与链接 boot loader 更加复杂。

操作系统内核通常在非常高的虚拟地址执行（如`0xf0100000`), 以便把处理器的虚拟地址空间的下半部分留给用户程序使用。

但是很多机器在这么高的地址处是没有物理内存的，那里不能存储内核。我们将使用处理器的内存管理硬件来将虚拟地址 `0xf0100000`（内核代码期待被运行的位置） 映射到物理地址 `0x00100000`(boot loader 把内核代码加载到的位置）。 这样内核的虚拟地址足够高，但是被加载到物理内存 1 MB 的位置，就在 BIOS 的上方。

现在，我们只需映射物理内存的前 4MB 就足够启动/运行了。使用`kern/entrypgdir.c`中手写的，静态初始化的页面目录 (page directory) 和页表 (page table) 来达到这个目的。

设备之间通过端口来进行通信。

**控制台输出**

> VGA:Video Graphics Array,视频图形阵列.

> 执行`make qemu`时,将在执行此命令的终端和qemu 自己的终端输出.为什么?

    答: JOS 内核被设置为把它自己的 console 输出不仅输出到虚拟 VGA 显示器(就是调用此命令的控制台),还输出到模拟 PC 的虚拟串口,也就是 QEMU 自己的标准输出. 类似地,内核不仅接受键盘输入,还接受串口输入.也就是两个窗口都可以输入.

1. 解释 `printf.c`和`console.c`之间的接口。`console.c`导出了那些函数？`printf.c`是怎样使用它们的？

    答： 如`console.h`所示，导出了下列函数：

    ```c
    void cons_init(void);
    int cons_getc(void);

    void kbd_intr(void); // irq 1
    void serial_intr(void); // irq 4
    ```
    `printf.c`中最终暴露出来的是`cprintf`函数.调用链是
    
    `printf.c` : `cprintf->vcprintf->putch->cputchar`

    ->`console.c`: `cputchar->cons_putc`

2. 解释`console.c`中的以下代码:

    ```c
    1      if (crt_pos >= CRT_SIZE) {
    2              int i;
    3              memmove(crt_buf, crt_buf + CRT_COLS, (CRT_SIZE - CRT_COLS) * sizeof(uint16_t));
    4              for (i = CRT_SIZE - CRT_COLS; i < CRT_SIZE; i++)
    5                      crt_buf[i] = 0x0700 | ' ';
    6              crt_pos -= CRT_COLS;
    7      }
    ```

    答: 







## 附记

### ELF 文件格式简述

ELF 文件包含两部分，ELF header 和 文件数据。文件数据又包含三部分：
- 程序头表 (program header table)
- 段头表 (section header table)
- 程序头表和段头表所引用的数据

**File Header**

ELF 文件头定义了使用 32 位地址还是 64 位地址。文件头大小在 32 位下是 52 个字节，64 位下是 64 个字节。

### 几个关键的魔鬼数字

1. 0x7c00

## 遗留问题

1. `main.c` 中的`ELFHDR->e_entry`跳转到了 elf 文件的入口点。如何验证？

## 参考资料
- [PPT: PC 硬件 与 x86 架构](https://pdos.csail.mit.edu/6.828/2018/lec/l-x86.pdf)
- [CSDN: Linux C 中内联汇编的语法格式及使用方法](https://blog.csdn.net/slvher/article/details/8864996)
- [知乎专栏：汇编入门](https://zhuanlan.zhihu.com/p/23902265)
- [常见 x86 汇编](http://www.cburch.com/csbsju/cs/350/handouts/x86.html)
- [main.c 代码分析](https://blog.csdn.net/xiaocainiaoshangxiao/article/details/22953279)
- [全局描述表 GDT](https://www.cnblogs.com/bajdcc/p/8972946.html)
- [ld 脚本语法教程](http://www.scoberlin.de/content/media/http/informatik/gcc_docs/ld_toc.html#TOC5)
- [lab1 博客](https://www.bbsmax.com/A/QW5Yq8B5ma/)
- [matrix 的博客：位运算技巧](http://www.matrix67.com/blog/archives/263)