## 环境初始化

```shell
mkdir ~/6828


```

## 练习三

阅读前文可知，启动扇区(boot sector)是在地址 `0x7c00` 处被加载的。在这个地址打断点，跟踪`boot/boot.S`中的代码，通过源代码和反汇编文件`obj/boot/boot.asm`来了解当前的执行位置。可以用GDB中的`x`/`i`命令来反汇编boot loader中的指令。

## gdb常用指令

