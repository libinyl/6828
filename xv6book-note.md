xv6 阅读笔记

# boot loader

## 时序和原理

1. 当 PC 启动时首先执行？
   
   BIOS.
2. BIOS 存储的位置？
   
   写死在母板上。
3. BIOS 的任务？
   
   预处理硬件环境，并把控制权教给 boot loader.
4. boot loader 的位置？
   
   启动盘第一个扇区的前 512 字节。
5. BIOS 把 boot loader 的指令读取到内存中的哪里？
   
   `0x7c00`地址，然后跳到这个地址（更改 ip 寄存器）开始执行。
6. 处理器刚开始的模式？
   
   实模式。Intel 8088. 只能工作在：16 位。
7. boot loader 的任务？
   
   把处理器从**实模式**切换到**保护模式**, 以加载 kernel.
8. 处理器的保护模式是？
   
   是现代的模式，工作在 32 位，用于加载 kernel.

## 代码详解

boot loader 的代码包含两部分：
   
   - `boot/asm.S`: 16 位和 32 位汇编混合代码
   - `boot/main.c`: C 代码

**汇编 bootstrap**

1. boot loader 的第一个指令是？
   
    `cli`.
2. `cli`的作用？
   
    去使能 CPU 中断 (interrupt).
3. 中断的作用？
   
   用于硬件设备调用操作系统函数（中断处理器，interrupt handlers).
4. 去使能 CPU 中断的原因？
   
   BIOS 就是个袖珍操作系统，为了初始化硬件可能会建立它自己的中断处理器。但 BIOS 已经将控制权转交给了 boot loader, BIOS 已经不再运行，所以继续运行 BIOS 的中断向量表已经不再安全。当 xv6 准备就绪，还会重新使能中断。
5. CPU 在实模式只有 8 个 16 位通用寄存器，但是处理器可以寻址 20 位内存。这是如何做到的？
   
   段寄存器`%cs,%ds,%es,%ss`提供了更强的寻址能力。当程序寻址 20 位地址时，addr = 16 * 段寄存器值 + 偏移地址。

6. 分别何时用到`%cs`,`%ds`,`%ss`寄存器？
   
   - 指令获取：`%cs`
   - 数据读写：`%ds`
   - 栈读写：`%ss`
  
7. 什么是线性地址，物理地址，逻辑地址？
   
    ![线性地址，物理地址，逻辑地址](/images/B-1.jpg)

   逻辑地址：包含一个段选择器和一个偏移量，通常协作`segment:offset`.
   
   线性地址：逻辑地址中的段由硬件控制（起到了"翻译"的作用）, 程序仅仅直接操作偏移量。如果使能了硬件分页，则线性地址被翻译为物理地址；否则 CPU 直接使用线性地址，将其作为物理地址来看待。
   
8. xv6 基于怎样的内存映射逻辑？
   
    - xv6 的 boot loader 没有使能硬件分页。
    - boot loader 使用的逻辑地址被硬件翻译为线性地址。
    - 此线性地址被 CPU 作为物理地址看待直接使用。

9. BIOS 去使能中断后做什么？
    
    BIOS 无法确定 `%ds,%es,%ss`的值，所以在去使能中断后的首要任务就是将`%ax`置零，然后复制到`%ds,%es,%ss`中。
10. 形如`segment:offset`的虚拟内存最大可以产生一个`0xffff*16H+0xffff=0x10ffef=1 0000 1111 1111 1110 1111`共 21 位的地址，而 Intel 8088 只有 20 位的寻址能力。Intel 8088 移除了这个最高位，导致这个偏移映射的地址不再是`0x10ffef`, 而是`0x0ffef`, 是某个一般的物理地址。早期的以来硬件的软件忽略了这个地址，那么当出现了高于 20 位的 CPU, 软件如何处理？boot loader 如何处理？
    
    IBM 提供了一个兼容性的技术，当键盘控制器的输出端口的第二位是`low`, 那么这个 21 位地址位总是清零的。如果是高，那么正常使用。

    boot loader 必须使能此 21 位地址。怎样实现？通过 I/O, 操作键盘控制器的`0x64`和 `0x60`端口。(boot.S 24~42 行）
11. 什么是段描述表 (segment descriptor table)? boot loader 是如何将处理器从 16 位切换到 32 位的？
    
    ![保护模式下的段](/images/B-2.jpg)

**C bootstrap**

1. C 代码部分的 bootstrap 的任务是什么？
   
   在磁盘上找到 kernel , 复制到内存。
2. 内核文件在哪？

    在硬盘上第二个扇区。

    
3. 如何加载内核？

   
   内核是一个 ELF 格式的二进制文件，需要读取其 ELF 头，然后根据其信息再做处理。

4. 如何查看内核文件的 ELF 信息？

    可通过`readelf -h kernel`查看内核的 elf header 信息；用`objdump -f kernel`查看入口点。

# 参考链接

- [scratch space](https://www.computerhope.com/jargon/s/scratch-space.htm)
- [ELF 文件](https://en.wikipedia.org/wiki/Executable_and_Linkable_Format)
- [使用 readelf 和 objdump 解析目标文件](https://www.jianshu.com/p/863b279c941e)