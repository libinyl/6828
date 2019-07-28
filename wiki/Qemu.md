# 与 QEMU 有关的信息


## [monitor](http://people.redhat.com/pbonzini/qemu-test-doc/_build/html/topics/pcsys_005fmonitor.html)

文档内搜索: info subcommand

- 切换到 qemu monitor : `Ctrl a c`
- 查看内存: x/[N]i [ADDR]  如 x/8i 0x00007c00

## 关键物理内存信息

- 第一条指令: `x/8i 0x000ffff0`  注意不是`0x0000fff0`
- boot loader断点: `b 0x7c00`


## QEMU 提供的物理内存是多少?

默认是 128MB.

> -m [size=]megs[,slots=n,maxmem=size]
> 
> Sets guest startup RAM size to megs megabytes. Default is 128 MiB. 

## QEMU 提供的物理内存硬件是什么?

## 参考[文档](https://qemu.weilnetz.de/doc/qemu-doc.html)†

