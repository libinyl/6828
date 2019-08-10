#include <inc/stab.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>

#include <kern/kdebug.h>
#include <kern/pmap.h>
#include <kern/env.h>

extern const struct Stab __STAB_BEGIN__[];	// Beginning of stabs table
extern const struct Stab __STAB_END__[];	// End of stabs table
extern const char __STABSTR_BEGIN__[];		// Beginning of string table
extern const char __STABSTR_END__[];		// End of string table

struct UserStabData {
	const struct Stab *stabs;
	const struct Stab *stab_end;
	const char *stabstr;
	const char *stabstr_end;
};

// stab_binsearch(stabs, region_left, region_right, type, addr)
//  有些 stab 类型按指令顺序升序排布.比如 N_FUN stabs (即进入 stab 段时类型为N_FUN)
//  就把函数标记为升序,N_SO stabs 把源文件标记为升序.
//	Some stab types are arranged in increasing order by instruction
//	address.  For example, N_FUN stabs (stab entries with n_type ==
//	N_FUN), which mark functions, and N_SO stabs, which mark source files.
//  
//  给定一个指令的地址,此函数找到包含此地址,且类型为'type'的 stab 入口点.
//	Given an instruction address, this function finds the single stab
//	entry of type 'type' that contains that address.

//  此函数在区间 [*region_left, *region_right] 搜索.
//  因此,如果想在一个数量为 N 的 stab 集合里搜索,可以这样做:
//	The search takes place within the range [*region_left, *region_right].
//	Thus, to search an entire set of N stabs, you might do:
//
//		left = 0;
//		right = N - 1;     /* 最右 stab */
//		stab_binsearch(stabs, &left, &right, type, addr);
//
//	The search modifies *region_left and *region_right to bracket the
//	'addr'.  *region_left points to the matching stab that contains
//	'addr', and *region_right points just before the next stab.  If
//	*region_left > *region_right, then 'addr' is not contained in any
//	matching stab.
//
//	For example, given these N_SO stabs:
//		Index  Type   Address
//		0      SO     f0100000
//		13     SO     f0100040
//		117    SO     f0100176
//		118    SO     f0100178
//		555    SO     f0100652
//		556    SO     f0100654
//		657    SO     f0100849
//	this code:
//		left = 0, right = 657;
//		stab_binsearch(stabs, &left, &right, N_SO, 0xf0100184);
//	will exit setting left = 118, right = 554.
//
static void
stab_binsearch(const struct Stab *stabs, int *region_left, int *region_right,
	       int type, uintptr_t addr)
{
	int l = *region_left, r = *region_right, any_matches = 0;

	while (l <= r) {
		int true_m = (l + r) / 2, m = true_m;

		// search for earliest stab with right type
		while (m >= l && stabs[m].n_type != type)
			m--;
		if (m < l) {	// no match in [l, m]
			l = true_m + 1;
			continue;
		}

		// actual binary search
		any_matches = 1;
		if (stabs[m].n_value < addr) {
			*region_left = m;
			l = true_m + 1;
		} else if (stabs[m].n_value > addr) {
			*region_right = m - 1;
			r = m - 1;
		} else {
			// exact match for 'addr', but continue loop to find
			// *region_right
			*region_left = m;
			l = m;
			addr++;
		}
	}

	if (!any_matches)
		*region_right = *region_left - 1;
	else {
		// find rightmost region containing 'addr'
		for (l = *region_right;
		     l > *region_left && stabs[l].n_type != type;
		     l--)
			/* do nothing */;
		*region_left = l;
	}
}


// (addr, info)
// 获取指定的指令地址 'addr' 的信息并填充到 'info' 结构.
// 如果找到信息,就返回 0,否则返回负数.
// 但即使返回负数,仍会向 '*info' 存储一些信息.
//
int
debuginfo_eip(uintptr_t addr, struct Eipdebuginfo *info)
{
	const struct Stab *stabs, *stab_end;
	const char *stabstr, *stabstr_end;
	int lfile, rfile, lfun, rfun, lline, rline;

	// 初始化 *info
	info->eip_file = "<unknown>";
	info->eip_line = 0;
	info->eip_fn_name = "<unknown>";
	info->eip_fn_namelen = 9;
	info->eip_fn_addr = addr;
	info->eip_fn_narg = 0;

	// 确定 stab 位置信息
	if (addr >= ULIM) {
		stabs = __STAB_BEGIN__;
		stab_end = __STAB_END__;
		stabstr = __STABSTR_BEGIN__;
		stabstr_end = __STABSTR_END__;
	} else {
		// The user-application linker script, user/user.ld,
		// puts information about the application's stabs (equivalent
		// to __STAB_BEGIN__, __STAB_END__, __STABSTR_BEGIN__, and
		// __STABSTR_END__) in a structure located at virtual address
		// USTABDATA.
		const struct UserStabData *usd = (const struct UserStabData *) USTABDATA;

		// Make sure this memory is valid.
		// Return -1 if it is not.  Hint: Call user_mem_check.
		// LAB 3: Your code here.

		stabs = usd->stabs;
		stab_end = usd->stab_end;
		stabstr = usd->stabstr;
		stabstr_end = usd->stabstr_end;

		// Make sure the STABS and string table memory is valid.
		// LAB 3: Your code here.
	}

	// String table 有效性校验
	if (stabstr_end <= stabstr || stabstr_end[-1] != 0)
		return -1;

	// 接下来要找到包含 eip 的函数所在的 stab.
	// 1. 找到包含 eip 的源文件
	// 2. 在此文件中找到目标函数
	// 3. 找到目标函数的行号

	// 在所有 stabs 里搜索源文件的 stab(类型是 N_SO)
	// lfile 和 rfilr 是包含源文件的 stabs 的索引值.
	// 参考: http://www.math.utah.edu/docs/info/stabs_12.html#SEC73
	lfile = 0;
	rfile = (stab_end - stabs) - 1;
	stab_binsearch(stabs, &lfile, &rfile, N_SO, addr);
	if (lfile == 0)
		return -1;

	// 在找到的 stab 中搜索与函数定义有关的 stab,类型是 N_FUN
	lfun = lfile;
	rfun = rfile;
	stab_binsearch(stabs, &lfun, &rfun, N_FUN, addr);

	// 参考: http://www.math.utah.edu/docs/info/stabs_12.html#SEC55
	if (lfun <= rfun) {
		// stabs[lfun] 指向 string table 里的函数名.
		// 但以防万一还是检查一下边界.
		if (stabs[lfun].n_strx < stabstr_end - stabstr)
			info->eip_fn_name = stabstr + stabs[lfun].n_strx;
		info->eip_fn_addr = stabs[lfun].n_value;
		addr -= info->eip_fn_addr;
		// Search within the function definition for the line number.
		lline = lfun;
		rline = rfun;
	} else {
		// Couldn't find function stab!  Maybe we're in an assembly
		// file.  Search the whole file for the line number.
		info->eip_fn_addr = addr;
		lline = lfile;
		rline = rfile;
	}
	// 确定函数名长度,忽略冒号之后的代码.
	info->eip_fn_namelen = strfind(info->eip_fn_name, ':') - info->eip_fn_name;
	if (lline <= rline) {
		info->eip_line = lline;
	}else{
		info->eip_line = -1;
	}

	// 在 [lline, rline] 中搜索与行号有关的 stab.
	// 如果找到了,就把 info->eip_line 的值设为行号.
	// 否则设为 -1.
	//
	// 提示:
	// 行号的类别请在 STABS documentation 和 <inc/stab.h>
	// 中寻找答案.
	// Your code here.
	stab_binsearch(stabs,&lline,&rline,N_SLINE,addr);
	if (lline <= rline) {
		info->eip_line = stabs[lline].n_desc;
	}else{
		info->eip_line = -1;
	}
	

	// Search backwards from the line number for the relevant filename
	// stab.
	// 从行号反向查找相应的文件名 stab.
	// We can't just use the "lfile" stab because inlined functions
	// can interpolate code from a different file!
	// 不能直接用 "lfile" stab,因为内联函数会从不同的文件中插入代码!
	// Such included source files use the N_SOL stab type.
	// 这类included source file(头文件?)使用 N_SOL stab 类型.
	while (lline >= lfile
	       && stabs[lline].n_type != N_SOL
	       && (stabs[lline].n_type != N_SO || !stabs[lline].n_value))
		lline--;
	if (lline >= lfile && stabs[lline].n_strx < stabstr_end - stabstr)
		info->eip_file = stabstr + stabs[lline].n_strx;


	// Set eip_fn_narg to the number of arguments taken by the function,
	// or 0 if there was no containing function.
	// 把 eip_fn_narg 设为函数接收的参数个数,如果没有则设为 0
	if (lfun < rfun)
		for (lline = lfun + 1;
		     lline < rfun && stabs[lline].n_type == N_PSYM;
		     lline++)
			info->eip_fn_narg++;

	return 0;
}
