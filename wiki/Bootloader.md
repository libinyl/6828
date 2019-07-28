## PC通电后第一个执行的指令地址在哪里?为什么?

1. 在`0x000ffff0`;
2. 因为 BIOS 被写死在了`0x000f0000-0x000fffff` 范围.

## BIOS 执行了什么?

1. 设置中断向量表;
2. 初始化VGA ,PCI BUS等设备;
3. 检测软/硬盘(floopy/hard disk),或者 CD-ROM;
4. 找到 bootable 磁盘后读取 bootloader,转移控制权.

## BIOS 是如何转移控制权的?

1. 找到第一个扇区即 boot sector;
2. 把boot sector的 512 个字节加载到物理内存的 [0x7c00 0x7dff] 处
3. 用 `jmp` 设置`CS:IP = 0000:7c00` 即`0x7c00`,开始执行 bootloader 的代码.(调试: b *0x7c00)

## BootLoader 的作用是什么?

1. 环境初始化
2. 加载内核文件到内存中
3. 转移控制权到内核初始化代码

## 汇编部分 BootLoader 具体做了什么?
1. CPU 切换至 32-bit 保护模式.
2. 通过 CPU 的 IO 寄存器访问磁盘
3. 设置C 执行环境
4. 跳转到 C 部分代码

## C 部分汇编做了什么?

1. 加载 elf 



## BootLoader 为什么要把 CPU 切换为保护模式?

## CPU 怎样切换至 32bit 保护模式?