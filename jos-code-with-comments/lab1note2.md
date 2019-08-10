# 详细调试日志

## 编译阶段

1. 编译内核

```
cd lab
make
```
----

## 上电阶段

2. QEMU 加载内核并开启 gdb:
```
cd lab;make qemu-nox-gdb
```
3. QEMU 切换至 monitor:
```
Ctrl+a,c
```
4. 查看 QEMU 加载(虚拟)磁盘状态:
```
(qemu) info block
ide0-hd0: obj/kern/kernel.img (raw)
    Cache mode:       writeback
```
5. 查看PCI信息:
```
(qemu) info pci
  Bus  0, device   0, function 0:
    Host bridge: PCI device 8086:1237
      id ""
  Bus  0, device   1, function 0:
    ISA bridge: PCI device 8086:7000
      id ""
  Bus  0, device   1, function 1:
    IDE controller: PCI device 8086:7010
      BAR4: I/O at 0xffffffffffffffff [0x000e].
      id ""
  Bus  0, device   1, function 3:
    Bridge: PCI device 8086:7113
      IRQ 0.
      id ""
  Bus  0, device   2, function 0:
    VGA controller: PCI device 1234:1111
      BAR0: 32 bit prefetchable memory at 0xffffffffffffffff [0x00fffffe].
      BAR2: 32 bit memory at 0xffffffffffffffff [0x00000ffe].
      BAR6: 32 bit memory at 0xffffffffffffffff [0x0000fffe].
      id ""
  Bus  0, device   3, function 0:
    Ethernet controller: PCI device 8086:100e
      IRQ 0.
      BAR0: 32 bit memory at 0xffffffffffffffff [0x0001fffe].
      BAR1: I/O at 0xffffffffffffffff [0x003e].
      BAR6: 32 bit memory at 0xffffffffffffffff [0x0003fffe].
      id ""
```

128MB的物理内存,如果每个字节一行,总共要输出 128\*1024\*1024=134217728(13 亿)行,全部输出不太现实.

6. 查看1MB 附近的内存,其上应该为空,其下是 BIOS.输出方向相反
```
(gdb) x/100x 0x000FFF00
0xfff00:	0x70e68ab0	0x316671e4	0x79c084d2	0x66536641
0xfff10:	0x00000fb8	0x58e86600	0x66ffff80	0x8ab0c389
0xfff20:	0x71e470e6	0x1679c084	0x66d88966	0xff7fc0e8
0xfff30:	0xc08566ff	0xe8660d75	0xffff963c	0x3166e0eb
0xfff40:	0x6604ebd2	0x66ffca83	0x5b66d089	0x8966c366
0xfff50:	0xcfc366d0	0x87f66866	0x00e90000	0x665666d6
0xfff60:	0xec836653	0xc389660c	0x7621e866	0x8966ffff
0xfff70:	0xc28966c6	0x10eac166	0xd2b60f66	0x438b6667
0xfff80:	0xdbe8660c	0x66ffffec	0x5a75c085	0x438b6667
0xfff90:	0x66672e0c	0x0014908b	0x6667fff1	0x00b5048d
0xfffa0:	0x66000000	0x8867d009	0x67052444	0x062444c6
0xfffb0:	0x0f666702	0x671053b7	0x07245488	0x2444c667
0xfffc0:	0xc6676c08	0xf6092444	0x02e2c166	0x448d6667
0xfffd0:	0x66670524	0x66240489	0x01754db9	0xd8896600
0xfffe0:	0xab9ae866	0x8366ffff	0x5b660cc4	0xc3665e66
0xffff0:	0x00e05bea	0x2f3630f0	0x392f3332	0x00fc0039
0x100000:	0x00000000	0x00000000	0x00000000	0x00000000
0x100010:	0x00000000	0x00000000	0x00000000	0x00000000
0x100020:	0x00000000	0x00000000	0x00000000	0x00000000
0x100030:	0x00000000	0x00000000	0x00000000	0x00000000
0x100040:	0x00000000	0x00000000	0x00000000	0x00000000
0x100050:	0x00000000	0x00000000	0x00000000	0x00000000
0x100060:	0x00000000	0x00000000	0x00000000	0x00000000
0x100070:	0x00000000	0x00000000	0x00000000	0x00000000
0x100080:	0x00000000	0x00000000	0x00000000	0x00000000
```


----

## BootLoader 阶段

7. 执行至 bootloader 第一个指令
```
b *0x7c00
```

8. 查看非 C 代码定义的符号(编译器附加的符号)

```
(gdb) info variables
All defined variables:

File kern/console.c:
static <unknown type> addr_6845;
static uint8_t *charcode[4];
static struct {
    uint8_t buf[512];
    uint32_t rpos;
    uint32_t wpos;
} cons;
...
Non-debugging symbols:
0xf0104eb0  __STAB_BEGIN__
0xf010af58  __STAB_END__
0xf010af59  __STABSTR_BEGIN__
0xf010cf21  __STABSTR_END__
0xf010d000  bootstack
0xf0115000  bootstacktop
0xf0117560  edata
0xf0117788  shift
0xf0117794  nextfree
0xf0117ba0  end
```

9. 查看当前所有函数

```
All defined functions:

File kern/console.c:
int cons_getc(void);
void cons_init(void);
void cputchar(int);
int getchar(void);
int iscons(int);
void kbd_intr(void);
void serial_intr(void);
```
