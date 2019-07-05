#include <inc/x86.h>
#include <inc/elf.h>

/**********************************************************************
 * This a dirt simple boot loader, whose sole job is to boot
 * an ELF kernel image from the first IDE hard disk.
 *
 * DISK LAYOUT
 *  * This program(boot.S and main.c) is the bootloader.  It should
 *    be stored in the first sector of the disk.
 *
 *  * The 2nd sector onward holds the kernel image.
 *
 *  * The kernel image must be in ELF format.
 *
 * BOOT UP STEPS
 *  * when the CPU boots it loads the BIOS into memory and executes it
 *
 *  * the BIOS intializes devices, sets of the interrupt routines, and
 *    reads the first sector of the boot device(e.g., hard-drive)
 *    into memory and jumps to it.
 *
 *  * Assuming this boot loader is stored in the first sector of the
 *    hard-drive, this code takes over...
 *
 *  * control starts in boot.S -- which sets up protected mode,
 *    and a stack so C code then run, then calls bootmain()
 *
 *  * bootmain() in this file takes over, reads in the kernel and jumps to it.
 **********************************************************************/

#define SECTSIZE	512

// scratch space, 磁盘上专门用于临时存储的空间. 如果内存不够一些大型软件溢出, 将在这里存放.
// 参考https://www.computerhope.com/jargon/s/scratch-space.htm
#define ELFHDR		((struct Elf *) 0x10000) 


void readsect(void*, uint32_t);
void readseg(uint32_t, uint32_t, uint32_t);

void
bootmain(void)
{
	// Proghdr是程序头,elf 文件中的数据部分,描述每个段的信息.
	// elf 中有一个头,多个(程序头+数据段头).每个程序头描述对应的段信息.
	// 
	struct Proghdr *ph, *eph;

	// 把底盘中的第一页读取进来.
	// 磁盘起始有scratch space 结束的位置.
	// 待完成文件系统后再来补充..
	readseg((uint32_t) ELFHDR, SECTSIZE*8, 0);

	// 验证 ELF 有效性(魔数发挥作用!)
	if (ELFHDR->e_magic != ELF_MAGIC)
		goto bad;

	// 把每一个程序段都加载到内存(忽略 ph 标记)
	// 初始化 ph 到第一个程序头位置
	ph = (struct Proghdr *) ((uint8_t *) ELFHDR + ELFHDR->e_phoff);
	// 初始化 eph 为段的数量
	eph = ph + ELFHDR->e_phnum;
	for (; ph < eph; ph++)
		// p_pa:		当前段的加载地址(也是物理地址),即需要放在内存中的目标位置
		// p_memsz:		当前段在内存中的大小
		// p_offset:	当前段在文件镜像中的偏移量
		readseg(ph->p_pa, ph->p_memsz, ph->p_offset);

	// 通过 elf header,调用内核入口,并永不反回
	((void (*)(void)) (ELFHDR->e_entry))();

bad:
	outw(0x8A00, 0x8A00);
	outw(0x8A00, 0x8E00);
	while (1)
		/* do nothing */;
}

// 从内核文件的 offset 开始,读取 cout 个 byte 到 内存中 pa 的位置.
// 实际可能比要求的复制更多一些.
void
readseg(uint32_t pa, uint32_t count, uint32_t offset)
{
	uint32_t end_pa;

	end_pa = pa + count;

	// round down to sector boundary
	pa &= ~(SECTSIZE - 1);

	// translate from bytes to sectors, and kernel starts at sector 1
	offset = (offset / SECTSIZE) + 1;

	// If this is too slow, we could read lots of sectors at a time.
	// We'd write more to memory than asked, but it doesn't matter --
	// we load in increasing order.
	while (pa < end_pa) {
		// Since we haven't enabled paging yet and we're using
		// an identity segment mapping (see boot.S), we can
		// use physical addresses directly.  This won't be the
		// case once JOS enables the MMU.
		readsect((uint8_t*) pa, offset);
		pa += SECTSIZE;
		offset++;
	}
}

void
waitdisk(void)
{
	// wait for disk reaady
	while ((inb(0x1F7) & 0xC0) != 0x40)
		/* do nothing */;
}

void
readsect(void *dst, uint32_t offset)
{
	// wait for disk to be ready
	waitdisk();

	outb(0x1F2, 1);		// count = 1
	outb(0x1F3, offset);
	outb(0x1F4, offset >> 8);
	outb(0x1F5, offset >> 16);
	outb(0x1F6, (offset >> 24) | 0xE0);
	outb(0x1F7, 0x20);	// cmd 0x20 - read sectors

	// wait for disk to be ready
	waitdisk();

	// read a sector
	insl(0x1F0, dst, SECTSIZE/4);
}

