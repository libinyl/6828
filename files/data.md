## kernel 数据

```
root@MyServer:~/6828/lab/obj/kern# objdump -h kernel

kernel:     file format elf32-i386

Sections:
Idx Name          Size      VMA       LMA       File off  Algn
  0 .text         00003f11  f0100000  00100000  00001000  2**4
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
  1 .rodata       00000f9b  f0103f14  00103f14  00004f14  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  2 .stab         000060a9  f0104eb0  00104eb0  00005eb0  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  3 .stabstr      00001fc9  f010af59  0010af59  0000bf59  2**0
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  4 .data         0000a544  f010d000  0010d000  0000e000  2**12
                  CONTENTS, ALLOC, LOAD, DATA
  5 .bss          00000654  f0117560  00117560  00018560  2**5
                  CONTENTS, ALLOC, LOAD, DATA
  6 .comment      00000035  00000000  00000000  00018bb4  2**0
                  CONTENTS, READONLY
```