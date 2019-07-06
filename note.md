# 调试笔记

## gdb 调试方式
1. `cd ~/6828/lab;make qemu-nox-gdb`
2. 在另一会话，`cd ~/6828/lab;make gdb`
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

4. 在 `0x7c00` 设置断点：`b *0x7c00`
5. `c` 让程序继续执行
6. `si` 汇编级执行下一指令
   

### gdb 常用功能速查

功能 | 按键
---|---
设置断点 | `b`
查看当前地址 | `where`
执行上次的命令 | 回车
查看汇编和寄存器 | [layout](https://blog.csdn.net/zhangjs0322/article/details/10152279)
查看寄存器 | `info registers` 或 简写 `i reg`
查看内存地址中的值 | [x](https://www.cnblogs.com/tekkaman/p/3506120.html)
跳进函数内部 (step into) | s
跳出函数 | [finish](https://sourceware.org/gdb/current/onlinedocs/gdb/Continuing-and-Stepping.html#Continuing-and-Stepping) (fin)

### gdb layout 速查

视图 | 命令
---|---
汇编视图 | layout asm
汇编+寄存器视图 | layout regs
源代码视图 | layout src

### gdb 远程调试步骤

1. 服务器安装 gdbserver, 本地机安装 gdb.
2. 端口映射：`ssh -L9091:localhost:9091 root@154.48.248.252`
3. 服务端编译程序，务必加上`-g`选项。如：`gcc -g data.c -o data`
4. 服务端运行：`gdbserver :9091 targetfile` 注意 gdbserver 后有空格。完整命令是`gdbserver localhost:9091 targetfile`
5. 本地运行：
    ```
    gdb
    target remote localhost:9091
    ```

## 常见位操作总结

```
// 判断奇偶性
if (x & 1 == 1) 则 x 为奇数
if (x & 1 == 0) 则 x 为偶数

// 
```

## 原码，补码，反码

1. 正数在内存中的值永远是其本身。
2. 补码是有符号数的表示方式，补码的值像一个圈，绕一圈再回来。

比如三位二进制有符号数字，从上到下，向两边正负延展。
```
    ↙      |      ↖               ↙      |      ↖
      000  |  111                     0  |   -1
  001      |      110             1      |       -2
↓          |          ↑       ↓          |          ↑
  010      |      101             2      |       -3
      011  |  100                     3  |   -4
    ↘      |      ↗               ↘      |      ↗
```

## 验证大小端系统

    ```c
    // 如果输出是按顺序的，即 123456ff, 则是大端，即数据增长方向与地址增长方向相同
    // 如果输出是反向的，即 ff563412, 则是小端，数据增长方向与地址增长方向相反
    int a = 0x123456ff;
    unsigned char *p = (unsigned char *)&a;
    printf("%x",*(p));
    printf("%x",*(p+1));
    printf("%x",*(p+2));
    printf("%x",*(p+3));
    ```

## 按位输出任意变量在内存中的二进制表示（假设是小端）

    ```c
    //assumes little endian
    void printBits(size_t const size, void const *const ptr)
    {
        unsigned char *b = (unsigned char *) ptr;
        unsigned char byte;

        int i, j;

        for (i = size - 1u; i >= 0; i--) {
            for (j = 7u; j >= 0; j--) {
                byte = (b[i] >> j) & 1;
                printf("%u", byte);
            }
        }
        puts("");
    }
    ```

## memmove 是什么，与 memcopy 有何差别？

答: 简而言之,通常情况下二者作用相同.但是在原字符串与目标字符串有重叠时, `memmove` 仍能正确复制,看起来像把原字符串复制到临时空间,再复制到目标空间,所以往往更加"强制性".而 memcopy 直接按位拷贝,性能会好,但不保证重叠时可以正确复制.

例:

```c
// include string.h, stdio.h, stdlib.h
int main(){
    char a[]="1234 5678 abcd efgh";

    char b[]="1234 5678 abcd efgh";

    memmove(a+5,a,20);
    puts(a);

    memcpy(b+5,b,20);
    puts(b);
}
// output
// 1234 1234 5678 abcd efgh        memmove 在正确性上更符合期望
// 1234 1234 5674 567d e67d        memcopy 在重叠时的行为是未定义的
```

## 常用信息

- qemu 未初始化的内存会被反编译为`add %al,(%bx,%si) `

## 遗留问题

- pc 寄存器 与 cs:ip 的区别？
- C 指针步进时，会自动增长其类型的长度。如何做到的？

## 其他问题

- ssh 解决超时问题：

    1. `vim /etc/ssh/sshd_config` , 添加一行 `ClientAliveInterval 60`
    2. 重启 ssh:`service ssh restart`

- 传输文件
    
    ```
    scp username@servername:/path/filename /Users/mac/Desktop（本地目录）
    ```

## 参考资料

[博客：gdb 远程调试](https://medium.com/@spe_/debugging-c-c-programs-remotely-using-visual-studio-code-and-gdbserver-559d3434fb78)