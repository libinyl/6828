/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>

# 右移逻辑 >> (todo: 需要再次确定)
#define SRL(val, shamt)		(((val) >> (shamt)) & ~(-1 << (32 - (shamt))))


###################################################################
# The kernel (this code) is linked at address ~(KERNBASE + 1 Meg), 
# but the bootloader loads it at address ~1 Meg.
#	
# RELOC(x) :把符号 x 从它的 链接地址 映射到它在物理内存(加载地址)中的实际位置.
###################################################################

#define	RELOC(x) ((x) - KERNBASE)

#define MULTIBOOT_HEADER_MAGIC (0x1BADB002)
#define MULTIBOOT_HEADER_FLAGS (0)
#define CHECKSUM (-(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS))

###################################################################
# entry point 入口点
###################################################################

.text

# The Multiboot header
.align 4
.long MULTIBOOT_HEADER_MAGIC
.long MULTIBOOT_HEADER_FLAGS
.long CHECKSUM

# '_start' 指定了 ELF 的入口点. 由于当 bootloader 执行此代码时,虚拟内存尚未
# 建立,所以我们需要让 bootloader 跳转到此入口点的 *物理* 地址.
.globl		_start
_start = RELOC(entry)

.globl entry
entry:
	movw	$0x1234,0x472			# warm boot

	# 由于尚未建立虚拟内存,所以当前代码运行在 kernel 加载的物理地址,
	# 也就是 1MB 的位置. (实际上是 1MB 加几个字节) .然而 C 代码被
	# 链接为在 KERNBASE+1MB 处执行.因此,我们建立一个相对不重要的
	# page directory,用于将虚拟地址的 [KERNBASE, KERNBASE+4MB)
	# 转换为物理地址的 [0, 4MB). 在建立真实的 page table
	# (实验 2中的mem_init)之前,这4MB 的区域是足够用的.

	# 把 entry_pgdir 的物理地址加载到 cr3. entry_pgdir 在 entrypgdir.c 中定义.

	# 开启映射:
	# 1. 把物理地址
	movl	$(RELOC(entry_pgdir)), %eax
	movl	%eax, %cr3
	# 开启 paging.
	movl	%cr0, %eax
	orl	$(CR0_PE|CR0_PG|CR0_WP), %eax
	movl	%eax, %cr0

	# 现在 paging 已经使能了,如果引用超出虚拟地址范围的地址立即回产生错误
	# 但是当前代码仍运行在一个较低的 EIP.为什么这是可行的?因为我们也把0~4MB 映射了,所以没有超出范围.这是一个平滑过渡.
	# 在进入 C 代码之前,跳过 KERNBASE.
	mov	$relocated, %eax
	jmp	*%eax
relocated:

	# 将 frame pointer register (EBP) 置零,
	# 这样,只要我们调试 C 代码(以调试模式进入), 栈调试器(tack backtraces)
	# 就可以合理地终止(todo:需要再确认)
	movl	$0x0,%ebp			# nuke frame pointer

	# Set the stack pointer
	movl	$(bootstacktop),%esp

	# 最后调用 C 代码
	call	i386_init

	# 正常情况不会运行到这里.
spin:	jmp	spin


.data
###################################################################
# boot stack :boot 阶段的栈
###################################################################
	.p2align	PGSHIFT		# force page alignment
	.globl		bootstack
bootstack:
	.space		KSTKSIZE
	.globl		bootstacktop   
bootstacktop:

