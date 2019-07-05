#include <inc/x86.h>
#include <inc/elf.h>

/**********************************************************************
 * 
 *  简陋的 boot loader 实现，任务就是启动IDE硬盘中的 ELF 内核镜像。
 *
 *
 * 硬盘布局
 *  * boot.S 和 main.c 就是 bootloader。它应该被存储在硬盘的第一个扇区。
 *
 *  * 第二个扇区存储内核镜像。
 *
 *  * 内核镜像必须是 ELF 格式。
 *
 *
 * 启动流程
 *
 *  * CPU 启动后，加载 BIOS 到内存中并执行；
 *
 *  * BIOS 初始化设备，一组中断程序，然后读取引导设备（比如引导盘）的第一个扇区
 *    到内存中，然后跳转到该地址。
 *
 *  * 假定此 boot loader 存储在引导盘的第一个扇区，那么此代码接管接下来的流程。
 *
 *  * 控制流从 boot.S 开始。boot.S 搭建好保护模式的环境，和用于 C 代码执行的栈，
 *    然后调用 bootmain() 函数。
 * 
 *  * 该文件的bootmain() 函数继续接管，读取内核并跳转过去。
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

