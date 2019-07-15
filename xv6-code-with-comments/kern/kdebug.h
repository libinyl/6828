#ifndef JOS_KERN_KDEBUG_H
#define JOS_KERN_KDEBUG_H

#include <inc/types.h>

// eip 的调试信息, 即 eip 指向指令所在的...
struct Eipdebuginfo {
	const char *eip_file;		// 源码文件
	int eip_line;				// 行号

	const char *eip_fn_name;	// 函数名.注: 不是"null terminated"字符串

	int eip_fn_namelen;			// 函数名长度
	uintptr_t eip_fn_addr;		// 函数起始点地址
	int eip_fn_narg;			// 函数参数个数
};

int debuginfo_eip(uintptr_t eip, struct Eipdebuginfo *info);

#endif
