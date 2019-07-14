// 简单的c 打印实现。作用是从内核输出至控制台。
// 基于 printfmt() 函数和内核 console 的 cputchar() 函数实现。


#include <inc/types.h>
#include <inc/stdio.h>
#include <inc/stdarg.h>


/**
 * 几个打印函数的调用关系:
 * 
 * 
 * cprintf()		: 最外层,边长参数初始化
 * ->vcprintf()		: 第二层,把 putch 函数作为回调传入 vprintfmt
 * ->vprintfmt()	: 格式化细节处理
 */

static void
putch(int ch, int *cnt)
{
	cputchar(ch);
	*cnt++;
}

int
vcprintf(const char *fmt, va_list ap)
{
	int cnt = 0;

	vprintfmt((void*)putch, &cnt, fmt, ap);
	return cnt;
}

// 最终暴露出来的格式化打印函数
// fmt: 格式化字符串
int
cprintf(const char *fmt, ...)
{
	va_list ap;//变长参数
	int cnt;

	va_start(ap, fmt);
	cnt = vcprintf(fmt, ap);
	va_end(ap);

	return cnt;
}

