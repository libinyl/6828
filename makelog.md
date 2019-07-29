```
root@MyServer:~/6828/lab# make
echo "   -O0 -fno-builtin -I. -MD -fno-omit-frame-pointer -std=gnu99 -static -Wall -Wno-format -Wno-unused -Werror -gstabs -m32 -fno-tree-ch -fno-stack-protector -DJOS_KERNEL -gstabs" | cmp -s obj/.vars.KERN_CFLAGS || echo "   -O0 -fno-builtin -I. -MD -fno-omit-frame-pointer -std=gnu99 -static -Wall -Wno-format -Wno-unused -Werror -gstabs -m32 -fno-tree-ch -fno-stack-protector -DJOS_KERNEL -gstabs" > obj/.vars.KERN_CFLAGS
+ as kern/entry.S
gcc -pipe -nostdinc    -O0 -fno-builtin -I. -MD -fno-omit-frame-pointer -std=gnu99 -static -Wall -Wno-format -Wno-unused -Werror -gstabs -m32 -fno-tree-ch -fno-stack-protector -DJOS_KERNEL -gstabs -c -o obj/kern/entry.o kern/entry.S
+ cc kern/entrypgdir.c
gcc -pipe -nostdinc    -O0 -fno-builtin -I. -MD -fno-omit-frame-pointer -std=gnu99 -static -Wall -Wno-format -Wno-unused -Werror -gstabs -m32 -fno-tree-ch -fno-stack-protector -DJOS_KERNEL -gstabs -c -o obj/kern/entrypgdir.o kern/entrypgdir.c
echo "" | cmp -s obj/.vars.INIT_CFLAGS || echo "" > obj/.vars.INIT_CFLAGS
+ cc kern/init.c
gcc -pipe -nostdinc    -O0 -fno-builtin -I. -MD -fno-omit-frame-pointer -std=gnu99 -static -Wall -Wno-format -Wno-unused -Werror -gstabs -m32 -fno-tree-ch -fno-stack-protector -DJOS_KERNEL -gstabs  -c -o obj/kern/init.o kern/init.c
+ cc kern/console.c
gcc -pipe -nostdinc    -O0 -fno-builtin -I. -MD -fno-omit-frame-pointer -std=gnu99 -static -Wall -Wno-format -Wno-unused -Werror -gstabs -m32 -fno-tree-ch -fno-stack-protector -DJOS_KERNEL -gstabs -c -o obj/kern/console.o kern/console.c
+ cc kern/monitor.c
gcc -pipe -nostdinc    -O0 -fno-builtin -I. -MD -fno-omit-frame-pointer -std=gnu99 -static -Wall -Wno-format -Wno-unused -Werror -gstabs -m32 -fno-tree-ch -fno-stack-protector -DJOS_KERNEL -gstabs -c -o obj/kern/monitor.o kern/monitor.c
+ cc kern/pmap.c
gcc -pipe -nostdinc    -O0 -fno-builtin -I. -MD -fno-omit-frame-pointer -std=gnu99 -static -Wall -Wno-format -Wno-unused -Werror -gstabs -m32 -fno-tree-ch -fno-stack-protector -DJOS_KERNEL -gstabs -c -o obj/kern/pmap.o kern/pmap.c
+ cc kern/kclock.c
gcc -pipe -nostdinc    -O0 -fno-builtin -I. -MD -fno-omit-frame-pointer -std=gnu99 -static -Wall -Wno-format -Wno-unused -Werror -gstabs -m32 -fno-tree-ch -fno-stack-protector -DJOS_KERNEL -gstabs -c -o obj/kern/kclock.o kern/kclock.c
+ cc kern/printf.c
gcc -pipe -nostdinc    -O0 -fno-builtin -I. -MD -fno-omit-frame-pointer -std=gnu99 -static -Wall -Wno-format -Wno-unused -Werror -gstabs -m32 -fno-tree-ch -fno-stack-protector -DJOS_KERNEL -gstabs -c -o obj/kern/printf.o kern/printf.c
+ cc kern/kdebug.c
gcc -pipe -nostdinc    -O0 -fno-builtin -I. -MD -fno-omit-frame-pointer -std=gnu99 -static -Wall -Wno-format -Wno-unused -Werror -gstabs -m32 -fno-tree-ch -fno-stack-protector -DJOS_KERNEL -gstabs -c -o obj/kern/kdebug.o kern/kdebug.c
+ cc lib/printfmt.c
gcc -pipe -nostdinc    -O0 -fno-builtin -I. -MD -fno-omit-frame-pointer -std=gnu99 -static -Wall -Wno-format -Wno-unused -Werror -gstabs -m32 -fno-tree-ch -fno-stack-protector -DJOS_KERNEL -gstabs -c -o obj/kern/printfmt.o lib/printfmt.c
+ cc lib/readline.c
gcc -pipe -nostdinc    -O0 -fno-builtin -I. -MD -fno-omit-frame-pointer -std=gnu99 -static -Wall -Wno-format -Wno-unused -Werror -gstabs -m32 -fno-tree-ch -fno-stack-protector -DJOS_KERNEL -gstabs -c -o obj/kern/readline.o lib/readline.c
+ cc lib/string.c
gcc -pipe -nostdinc    -O0 -fno-builtin -I. -MD -fno-omit-frame-pointer -std=gnu99 -static -Wall -Wno-format -Wno-unused -Werror -gstabs -m32 -fno-tree-ch -fno-stack-protector -DJOS_KERNEL -gstabs -c -o obj/kern/string.o lib/string.c
echo "-m elf_i386 -T kern/kernel.ld -nostdlib" | cmp -s obj/.vars.KERN_LDFLAGS || echo "-m elf_i386 -T kern/kernel.ld -nostdlib" > obj/.vars.KERN_LDFLAGS
+ ld obj/kern/kernel
ld -o obj/kern/kernel -m elf_i386 -T kern/kernel.ld -nostdlib obj/kern/entry.o obj/kern/entrypgdir.o obj/kern/init.o obj/kern/console.o obj/kern/monitor.o obj/kern/pmap.o obj/kern/kclock.o obj/kern/printf.o obj/kern/kdebug.o  obj/kern/printfmt.o  obj/kern/readline.o  obj/kern/string.o /usr/lib/gcc/i686-linux-gnu/5/libgcc.a -b binary
ld: warning: section `.bss' type changed to PROGBITS
objdump -S obj/kern/kernel > obj/kern/kernel.asm
nm -n obj/kern/kernel > obj/kern/kernel.sym
+ as boot/boot.S
gcc -pipe -nostdinc    -O0 -fno-builtin -I. -MD -fno-omit-frame-pointer -std=gnu99 -static -Wall -Wno-format -Wno-unused -Werror -gstabs -m32 -fno-tree-ch -fno-stack-protector -DJOS_KERNEL -gstabs -c -o obj/boot/boot.o boot/boot.S
+ cc -Os boot/main.c
gcc -pipe -nostdinc    -O0 -fno-builtin -I. -MD -fno-omit-frame-pointer -std=gnu99 -static -Wall -Wno-format -Wno-unused -Werror -gstabs -m32 -fno-tree-ch -fno-stack-protector -DJOS_KERNEL -gstabs -Os -c -o obj/boot/main.o boot/main.c
+ ld boot/boot
ld -m elf_i386 -N -e start -Ttext 0x7C00 -o obj/boot/boot.out obj/boot/boot.o obj/boot/main.o
objdump -S obj/boot/boot.out >obj/boot/boot.asm
objcopy -S -O binary -j .text obj/boot/boot.out obj/boot/boot
perl boot/sign.pl obj/boot/boot
boot block is 390 bytes (max 510)
+ mk obj/kern/kernel.img
dd if=/dev/zero of=obj/kern/kernel.img~ count=10000 2>/dev/null
dd if=obj/boot/boot of=obj/kern/kernel.img~ conv=notrunc 2>/dev/null
dd if=obj/kern/kernel of=obj/kern/kernel.img~ seek=1 conv=notrunc 2>/dev/null
mv obj/kern/kernel.img~ obj/kern/kernel.img
```