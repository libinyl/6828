## 初始化实验环境

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
## [Lab 1 Booting a PC](https://pdos.csail.mit.edu/6.828/2018/labs/lab1/)

### Part1 启动 PC

1. 编译内核
    ```shell
    cd lab
    make
    ```

    生成了文件`obj/kern/kernel.img`, 就是我们的虚拟**硬盘**. 这个"硬盘"镜像中包含了 **boot loader** (obj/boot/boot) 和内核 **obj/kernel** . 下一步，启动 QEMU, 加载硬盘。

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

### Part2  Boot Loader

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

详细分析可参考这篇 [博客](https://blog.csdn.net/cinmyheart/article/details/39762595)

```shell
git clone git://github.com/mit-pdos/xv6-public.git
cd ~/xv6-public
make
```

PPT: [PC 硬件 与 x86 架构](https://pdos.csail.mit.edu/6.828/2018/lec/l-x86.pdf)

## 附 linux 常用命令

- 查看电脑是多少位

`getconf LONG_BIT`

- 从本地上传到服务器

`scp 源 宿`

- 解决 ssh 自动断开问题
  
  `sudo vim /etc/ssh/sshd_config`, 去掉`ServerAliveInterval`前的注释，并将值改为 60. 参考 [链接](https://blog.csdn.net/moxiaomomo/article/details/52982047).
