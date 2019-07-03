## 初始化实验环境

1. 直接执行 [initlab.sh](initlab.sh), 下载实验所需代码
2. 初始化 `QEMU`:

    配置 qemu

    ```
    cd qemu
    ./configure --disable-kvm --disable-werror  --target-list="i386-softmmu x86_64-softmmu"
    ```

    如果配置 qemu 时缺少包（在 initlab.sh 中应已执行）:

    ```
    apt install pkg-config libglib2.0-dev zlib1g-dev libpixman-1-dev
    ```

    编译安装 qemu
    ```
    make && make install
    ```
## [Lab 1: Booting a PC](https://pdos.csail.mit.edu/6.828/2018/labs/lab1/)

1. 编译内核

    ```
    cd lab
    make
    ```

    生成了文件`obj/kern/kernel.img`, 就是我们的虚拟**硬盘**. 这个"硬盘"镜像中包含了 **boot loader** (obj/boot/boot) 和内核 **obj/kernel** . 下一步，启动 QEMU, 加载硬盘。

2. 启动 QEMU
   
    
    ```
    cd lab
    make qemu-nox
    ```

    此命令启动 QEMU,将输出

    输出:

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
    退出方式:`Ctrl+a`(松手)`x`.

    此时只有两个有效命令:`help`和`kerninfo`

    




```
git clone git://github.com/mit-pdos/xv6-public.git
cd ~/xv6-public
make
```

PPT: [PC 硬件 与 x86 架构](https://pdos.csail.mit.edu/6.828/2018/lec/l-x86.pdf)

## 附:linux 常用命令

- 查看电脑是多少位

`getconf LONG_BIT`

- 从本地上传到服务器

`scp 源 宿`
