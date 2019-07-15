// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
};

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(commands); i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

/**
 * 练习 9 输出描述: 
 * 
 * Stack backtrace:
 *   ebp f0109e58  eip f0100a62  args 00000001 f0109e80 f0109e98 f0100ed2 00000031	// 第一行显示当前正在执行的函数的信息
 *   ebp f0109ed8  eip f01000d6  args 00000000 00000000 f0100058 f0109f28 00000061  // 第二行显示调用当前函数的函数的信息
 *   ...																			// 以此类推,打印出所有的栈帧. 可参考 entry.S 来确定"所有".
 * 
 * ebp : 进入函数时,call 指令将父函数的 esp 的值复制给了 ebp.
 * eip : 函数的返回指针(return instruction pointer).当函数执行完毕并执行了 return后下一条要执行的命令的地址.
 *       通常指向 call 指令之后的指令.
 * args: 传入函数的前 5 个参数.注意,当给定的参数数量少于 5 个时,显示出来的 5 个值肯定有几个是没用的.
 * 
 * 
 * 练习 10 输出描述:
 * 
 * > backtrace
 * Stack backtrace:
 *   ebp f010ff78  eip f01008ae  args 00000001 f010ff8c 00000000 f0110580 00000000
 *       kern/monitor.c:143: monitor+106
 *   ebp f010ffd8  eip f0100193  args 00000000 00001aac 00000660 00000000 00000000
 *       kern/init.c:49: i386_init+59
 * 
 *       [文件]:[行号]: [函数名]+[函数返回时 eip 偏移量](offset,以字节为单位)
 */

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
    // Your code here.
    cprintf("Stack backtrace:\n");
    uint32_t *ebp =(uint32_t *) read_ebp();
    while(0!=ebp)
    {
        cprintf("  ebp  %08x  eip %08x  args %08x\t%08x\t%08x\t%08x\t%08x\n",ebp,ebp[1],ebp[2],ebp[3],ebp[4],ebp[5],ebp[6],ebp[7]);
		struct Eipdebuginfo info;
		debuginfo_eip(ebp,&info);
		cprintf("info.eip_file:%s\n",info.eip_file);
        ebp=(uint32_t *)*ebp;
    }
    return 0;
}



/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");


	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
