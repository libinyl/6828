## 调试笔记
1. `cd ~/6828/lab;make qemu-nox-gdb`
2. 在另一会话,`cd ~/6828/lab;make gdb`
3. `(gdb)layout asm`汇编查看窗口
4. 切换汇编风格：
    ```
    set disassembly-flavor intel
    set disassembly-flavor att
    ```

4. 查看即将执行的指令：
    ```
    (gdb) x/10i $cs*0x10+$eip
          0xffff0: 	ljmp   $0xf000,$0xe05b
    ```

4. 在 `0x7c00` 设置断点:`b *0x7c00`
5. `c` 让程序继续执行
6. `si` 汇编级执行下一指令
   

### gdb 常用命令

功能 | 按键
---|---
设置断点 | b
查看当前地址 | where
执行上次的命令 | 回车
查看汇编和寄存器 | [layout](https://blog.csdn.net/zhangjs0322/article/details/10152279)
查看寄存器 | info registers 或 简写 i reg
查看内存地址中的值 | [x](https://www.cnblogs.com/tekkaman/p/3506120.html)

汇编视图 layout asm
汇编+寄存器视图 layout regs
更新视图

## 常用信息

- qemu 未初始化的内存会被反编译为`add %al,(%bx,%si) `

## 遗留问题

pc 寄存器 与 cs:ip 的区别？