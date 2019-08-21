/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/mmu.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/pmap.h>
#include <kern/kclock.h>
#include <kern/env.h>

// 在 i386_detect_memory()中初始化:
size_t npages;			// 物理内存页数
static size_t npages_basemem;	// 物理内存中 base memory 部分的页数


// 这三个变量在mem_init()中赋值.
pde_t *kern_pgdir;						// 内核的初始 page directory
struct PageInfo *pages;					// 物理页信息数组
static struct PageInfo *page_free_list;	// 可用物理页边界,指向上一个 free 的 page


// --------------------------------------------------------------
// 检测机器的物理内存设置.
// --------------------------------------------------------------

static int
nvram_read(int r)
{
	return mc146818_read(r) | (mc146818_read(r + 1) << 8);
}

// 计算可用物理内存页数
static void
i386_detect_memory(void)
{
	size_t basemem, extmem, ext16mem, totalmem;

	// Use CMOS calls to measure available base & extended memor.y
	// (CMOS calls return results in kilobytes.)
	basemem = nvram_read(NVRAM_BASELO);
	extmem = nvram_read(NVRAM_EXTLO);
	ext16mem = nvram_read(NVRAM_EXT16LO) * 64;

	// Calculate the number of physical pages available in both base
	// and extended memory.
	if (ext16mem)
		totalmem = 16 * 1024 + ext16mem;
	else if (extmem)
		totalmem = 1 * 1024 + extmem;
	else
		totalmem = basemem;

	npages = totalmem / (PGSIZE / 1024);		// 页数=总字节数(KB)/每页有多少KB字节   npages=32768
	npages_basemem = basemem / (PGSIZE / 1024);	// npages_basemem=160

	// 总物理内存 = 基本内存 + 扩展内存
	cprintf("Physical memory: %uK available, base = %uK, extended = %uK\n",
		totalmem, basemem, totalmem - basemem);
	// Physical memory: 131072K available, base = 640K, extended = 130432K
	// Physical memory: 128MB available, base = 640K, extended = 130432K
}


// --------------------------------------------------------------
// 建立 UTOP 之上的内存映射,即用户不可写入的部分
// --------------------------------------------------------------

static void boot_map_region(pde_t *pgdir, uintptr_t va, size_t size, physaddr_t pa, int perm);
static void check_page_free_list(bool only_low_memory);
static void check_page_alloc(void);
static void check_kern_pgdir(void);
static physaddr_t check_va2pa(pde_t *pgdir, uintptr_t va);
static void check_page(void);
static void check_page_installed_pgdir(void);

/**
 * 物理内存申请器(以字节为单位)
 *  说明: 仅限建立虚拟内存时使用.实际是nextfree初始化程序.
 * 
 * 1) 若 n > 0, 申请 n 个字节的物理地址,4K 对齐.(仅限建立虚拟内存时使用)
 * 	  由 nextfree 维护申请状态.
 * 		返回值		: 指向新申请的内存块的指针 	
 * 		更改全局状态 : nextfree
 * 
 * 		异常结果
 * 			内存用尽: 触发 panic.
 * 
 * 2) 若 n = 0, 返回下一个可用的空闲页地址(内核虚拟地址)
 */
static void *
boot_alloc(uint32_t n)
{
	static char *nextfree;	// 下一个空闲的字节的虚拟地址
	char *result;

	if (!nextfree) {
		extern char end[];// linker 自动生成的符号,指向 bss 段的结尾(bss 顺序向上).也就是 linker 到目前为止还没分配给任何内核代码或全局变量的地址. 0xf0117ba0
		nextfree = ROUNDUP((char *) end, PGSIZE);// 从 end 开始,向上取整PGSIZE的整数倍,作为第一个可用的虚拟地址.思考方式:end 已经接近我们的目标值,此步骤就是对 end 做一个微小的调整.
	}

	if(n==0)
		return nextfree;
	
	if ( n>0 ){
		result = nextfree;		//注意,返回的是指向这块内存初始位置的地址,也就是 nextfree 在增长之前的值.可用地址:[result,nextfree)
		nextfree += ROUNDUP(n,PGSIZE);
	}
	return result;
}

/**
 * 建立地址空间内核部分的二级页表(即 UTOP 以上的部分)
 * 	[UTOP, ULIM): 用户可读,不可写
 *  [ULIM, ...]	: 用户不可读写
 * 		
 */ 
void
mem_init(void)
{
	uint32_t cr0;
	size_t n;

	// 1. 计算物理内存页数 设置状态:npages, npages_basemem
	i386_detect_memory();

	// 2. 创建内核 page directory
	kern_pgdir = (pde_t *) boot_alloc(PGSIZE);
	memset(kern_pgdir, 0, PGSIZE);

	// 3. 把内核页表本身的物理地址映射到UVPT,即用户虚拟页表. 权限: kernel R, user R
	kern_pgdir[PDX(UVPT)] = PADDR(kern_pgdir) | PTE_U | PTE_P;

	// 4. 申请一块内存, 承载存储物理页信息数组.
	pages = boot_alloc(npages * sizeof (struct PageInfo));
	memset(pages, 0, npages * sizeof(struct PageInfo));

	// 5. 申请一块内存, 承载环境信息数组.
	envs = (struct Env *) boot_alloc(NENV * sizeof(struct Env));
    memset(envs, 0, NENV * sizeof(struct Env));

	// 6. 初始化物理页信息,维护 page_free_list
	page_init();

	check_page_free_list(1);
	check_page_alloc();
	check_page();

	//////////////////////////////////////////////////////////////////////
	// 建立虚拟内存.

	// 把 pages对应的物理内存 映射值 UPAGES 处, 用户只读.
	boot_map_region(kern_pgdir, (uintptr_t) UPAGES, npages*sizeof(struct PageInfo), PADDR(pages), PTE_U | PTE_P);

	// 把 env 数组对应的物理内存映射至 UENVS,用户只读.
    boot_map_region(kern_pgdir, (uintptr_t) UENVS, ROUNDUP(NENV*sizeof(struct Env), PGSIZE), PADDR(envs), PTE_U | PTE_P);

	// 把 bootstack 作为内核栈,映射至[KSTACKTOP-PTSIZE, KSTACKTOP).
	boot_map_region(kern_pgdir, (uintptr_t) (KSTACKTOP-KSTKSIZE), KSTKSIZE, PADDR(bootstack), PTE_W | PTE_P);

	// 映射 KERNBASE 以上的所有内存.
	boot_map_region(kern_pgdir, (uintptr_t) KERNBASE, ROUNDUP(0xffffffff - KERNBASE, PGSIZE), 0, PTE_W | PTE_P);

	check_kern_pgdir();

	// Switch from the minimal entry page directory to the full kern_pgdir
	// page table we just created.	Our instruction pointer should be
	// somewhere between KERNBASE and KERNBASE+4MB right now, which is
	// mapped the same way by both page tables.
	//
	// 从 minimal entry page directory 切换到 full kern_pgdir page.
	// 指令指针寄存器应该位于 KERNBASE and KERNBASE+4MB .
	// If the machine reboots at this point, you've probably set up your
	// kern_pgdir wrong.
	lcr3(PADDR(kern_pgdir));

	check_page_free_list(0);

	// entry.S set the really important flags in cr0 (including enabling
	// paging).  Here we configure the rest of the flags that we care about.
	// 设置其他的控制位
	cr0 = rcr0();
	cr0 |= CR0_PE|CR0_PG|CR0_AM|CR0_WP|CR0_NE|CR0_MP;
	cr0 &= ~(CR0_TS|CR0_EM);
	lcr0(cr0);

	// Some more checks, only possible after kern_pgdir is installed.
	check_page_installed_pgdir();
}

// --------------------------------------------------------------
// 维护物理页信息.
// 对每个物理页,page 数组都有一个 PageInfo 结构与其对应.
// Page 被引用计数,空闲页被用一个链表维护.
// --------------------------------------------------------------

/**
 * 
 * 初始化物理页信息,初始化 page_free_list
 * 	设置状态: 
 * 		1) pages
 * 		2) page_free_list
 */ 
void
page_init(void)
{
	// 1. 标记 page[0] 为 use 状态.
	pages[0].pp_ref = 1;
	pages[0].pp_link = NULL;

	// 2. base memory [PGSIZE, npages_basemem * PGSIZE) 的剩余部分是 free 的.
	// 让每个 page 元素都可以指向上一个(空闲的)元素.
	size_t i;
	for (i = 1; i < npages_basemem; i++) {
		pages[i].pp_ref = 0;
		pages[i].pp_link = page_free_list;//当前page指向上一个空闲的 page.对于最后一个节点,相当于 node.next = NULL
		page_free_list = &pages[i];
	}

	// 3. IO 空洞[IOPHYSMEM, EXTPHYSMEM),把每个 page 都标记为 use 状态. 
	for (i = IOPHYSMEM/PGSIZE; i < EXTPHYSMEM/PGSIZE; i++ )
		pages[i].pp_ref = 1;

	// 4. 扩展内存 [EXTPHYSMEM, ...).
	//    跳过remapped pa mem.
    size_t first_free_address = PADDR(boot_alloc(0));
    for (i = EXTPHYSMEM/PGSIZE; i < first_free_address/PGSIZE; i++) {
        pages[i].pp_ref = 1;
    }
    for (i = first_free_address/PGSIZE; i < npages; i++) {
        pages[i].pp_ref = 0;
        pages[i].pp_link = page_free_list;
        page_free_list = &pages[i];
    }
}

/**
 * 物理内存申请器(以页为单位).
 * 	选项: 如果(alloc_flags & ALLOC_ZERO)==1,那么把返回的页填充为 0.
 *  说明: 只进行分配,不增加计数
 * 
 */ 
struct PageInfo *
page_alloc(int alloc_flags)
{
	// Fill this function in
	struct PageInfo *pg = page_free_list;
	if(pg == NULL)
		return NULL;

	page_free_list = pg->pp_link;

	pg->pp_ref = 0;
	pg->pp_link = NULL;

	if(alloc_flags & ALLOC_ZERO)
		memset(page2kva(pg),'\0',PGSIZE);
	return pg;
}

//
// Return a page to the free list.
// (This function should only be called when pp->pp_ref reaches 0.)
//
void
page_free(struct PageInfo *pp)
{
	// Fill this function in
	// Hint: You may want to panic if pp->pp_ref is nonzero or
	// pp->pp_link is not NULL.
	if(pp->pp_ref||pp->pp_link)
		panic("pp->pp_ref is nonzero or pp->pp_link is not NULL.");

	struct PageInfo *tmp = page_free_list;
	page_free_list = pp;
	pp->pp_link = tmp;
}

//
// Decrement the reference count on a page,
// freeing it if there are no more refs.
//
void
page_decref(struct PageInfo* pp)
{
	if (--pp->pp_ref == 0)
		page_free(pp);
}

/**
 * 定位虚拟地址 va 对应的 page table entry.
 * 选项: 若对应的 entry 不存在,则判断参数 create.若为 true,则创建并返回.
 * 
 */ 
pte_t *
pgdir_walk(pde_t *pgdir, const void *va, int create)
{		
    // 参数1: 页目录项指针
    // 参数2: 线性地址，JOS 中等于虚拟地址
    // 参数3: 若页目录项不存在是否创建
    // 返回: 页表项指针
    uint32_t page_dir_idx = PDX(va);
    uint32_t page_tab_idx = PTX(va);
    pte_t *pgtab;
    if (pgdir[page_dir_idx] & PTE_P) {
        pgtab = KADDR(PTE_ADDR(pgdir[page_dir_idx]));
    } else {
        if (create) {
            struct PageInfo *new_pageInfo = page_alloc(ALLOC_ZERO);
            if (new_pageInfo) {
                new_pageInfo->pp_ref += 1;
                pgtab = (pte_t *) page2kva(new_pageInfo);
                // 修改页目录的flag，根据 check_page 函数中用到的属性。
                // 因为分配以页为单位对齐，必然后 12bit 为0
                pgdir[page_dir_idx] = PADDR(pgtab) | PTE_P | PTE_W | PTE_U;
            } else {
                return NULL;
            }
        } else {
            return NULL;
        }
    }
    return &pgtab[page_tab_idx];

}

// 通过内核 page directory, 把虚拟地址[va, va+size) 处的虚拟地址映射值物理地址 [pa, pa+size) 处.
// 	说明: 只做映射,不改计数.
static void
boot_map_region(pde_t *pgdir, uintptr_t va, size_t size, physaddr_t pa, int perm)
{
    pte_t *pgtab;
    size_t pg_num = PGNUM(size);
    cprintf("map region size = %d, %d pages\n",size, pg_num);
    for (size_t i=0; i<pg_num; i++) {
        pgtab = pgdir_walk(pgdir, (void *)va, 1);
        if (!pgtab) {
            return;
        }
        //cprintf("va = %p\n", va);
        *pgtab = pa | perm | PTE_P;
        va += PGSIZE;
        pa += PGSIZE;
    }
}

/**
 * 
 * 把 pageInfo pp 对应的物理页映射到虚拟地址 va 处.
 */ 
int
page_insert(pde_t *pgdir, struct PageInfo *pp, void *va, int perm)
{
    pte_t *pgtab = pgdir_walk(pgdir, va, 1);  // 查找该虚拟地址对应的页表项，不存在则建立。
    if (!pgtab) {
        return -E_NO_MEM;  // 空间不足
    }
    if (*pgtab & PTE_P) {
        // 页表项已经存在，即该虚拟地址已经映射到物理页了
        if (page2pa(pp) == PTE_ADDR(*pgtab)) {
            // 如果映射到与之前相同的页，仅更改权限，不增加引用
            // 记录自己犯的一个错误，这种写法无法减少权限
            // *pgtab |= perm;
            *pgtab = page2pa(pp) | perm | PTE_P;
            return 0;
        } else {
            // 如果是更新映射的物理页，则要删除之前的映射关系
            page_remove(pgdir, va);
        }
    }
    *pgtab = page2pa(pp) | perm | PTE_P;
    pp->pp_ref++;
    return 0;
}

//
// Return the page mapped at virtual address 'va'.
// If pte_store is not zero, then we store in it the address
// of the pte for this page.  This is used by page_remove and
// can be used to verify page permissions for syscall arguments,
// but should not be used by most callers.
//
// Return NULL if there is no page mapped at va.
//
// Hint: the TA solution uses pgdir_walk and pa2page.
//
// 返回被映射到 虚拟地址'va' 的 page.
// 如果pte_store非 0,就把
struct PageInfo *
page_lookup(pde_t *pgdir, void *va, pte_t **pte_store)
{
    // 参数1: 页目录指针
    // 参数2: 线性地址，JOS 中等于虚拟地址
    // 参数3: 指向页表指针的指针
    // 返回: 页描述结构体指针
    pte_t *pgtab = pgdir_walk(pgdir, va, 0);  // 不创建，只查找
    if (!pgtab) {
        return NULL;  // 未找到则返回 NULL
    }
    if (pte_store) {
        *pte_store = pgtab;  // 附加保存一个指向找到的页表的指针
    }
    return pa2page(PTE_ADDR(*pgtab));  //  返回页面描述
}

//
// Unmaps the physical page at virtual address 'va'.
// If there is no physical page at that address, silently does nothing.
//
// Details:
//   - The ref count on the physical page should decrement.
//   - The physical page should be freed if the refcount reaches 0.
//   - The pg table entry corresponding to 'va' should be set to 0.
//     (if such a PTE exists)
//   - The TLB must be invalidated if you remove an entry from
//     the page table.
//
// Hint: The TA solution is implemented using page_lookup,
// 	tlb_invalidate, and page_decref.
//
void
page_remove(pde_t *pgdir, void *va)
{
    // Fill this function in
    pte_t *pgtab;
    pte_t **pte_store = &pgtab;
    struct PageInfo *pInfo = page_lookup(pgdir, va, pte_store);
    if (!pInfo) {
        return;
    }
    page_decref(pInfo);
    *pgtab = 0;  // 将内容清0，即无法再根据页表内容得到物理地址。
    tlb_invalidate(pgdir, va);  // 通知tlb失效。tlb是个高速缓存，用来缓存查找记录增加查找速度。
}

//
// Invalidate a TLB entry, but only if the page tables being
// edited are the ones currently in use by the processor.
//
void
tlb_invalidate(pde_t *pgdir, void *va)
{
	// Flush the entry only if we're modifying the current address space.
	// For now, there is only one address space, so always invalidate.
	invlpg(va);
}

static uintptr_t user_mem_check_addr;

//
// Check that an environment is allowed to access the range of memory
// [va, va+len) with permissions 'perm | PTE_P'.
// Normally 'perm' will contain PTE_U at least, but this is not required.
// 'va' and 'len' need not be page-aligned; you must test every page that
// contains any of that range.  You will test either 'len/PGSIZE',
// 'len/PGSIZE + 1', or 'len/PGSIZE + 2' pages.
//
// A user program can access a virtual address if (1) the address is below
// ULIM, and (2) the page table gives it permission.  These are exactly
// the tests you should implement here.
//
// If there is an error, set the 'user_mem_check_addr' variable to the first
// erroneous virtual address.
//
// Returns 0 if the user program can access this range of addresses,
// and -E_FAULT otherwise.
//
int
user_mem_check(struct Env *env, const void *va, size_t len, int perm)
{
	// LAB 3: Your code here.

	return 0;
}

//
// Checks that environment 'env' is allowed to access the range
// of memory [va, va+len) with permissions 'perm | PTE_U | PTE_P'.
// If it can, then the function simply returns.
// If it cannot, 'env' is destroyed and, if env is the current
// environment, this function will not return.
//
void
user_mem_assert(struct Env *env, const void *va, size_t len, int perm)
{
	if (user_mem_check(env, va, len, perm | PTE_U) < 0) {
		cprintf("[%08x] user_mem_check assertion failure for "
			"va %08x\n", env->env_id, user_mem_check_addr);
		env_destroy(env);	// may not return
	}
}


// --------------------------------------------------------------
// 检测函数.
// --------------------------------------------------------------

//
// Check that the pages on the page_free_list are reasonable.
//
static void
check_page_free_list(bool only_low_memory)
{
	struct PageInfo *pp;
	unsigned pdx_limit = only_low_memory ? 1 : NPDENTRIES;
	int nfree_basemem = 0, nfree_extmem = 0;
	char *first_free_page;

	if (!page_free_list)
		panic("'page_free_list' is a null pointer!");

	if (only_low_memory) {
		// Move pages with lower addresses first in the free
		// list, since entry_pgdir does not map all pages.
		struct PageInfo *pp1, *pp2;
		struct PageInfo **tp[2] = { &pp1, &pp2 };
		for (pp = page_free_list; pp; pp = pp->pp_link) {
			int pagetype = PDX(page2pa(pp)) >= pdx_limit;
			*tp[pagetype] = pp;
			tp[pagetype] = &pp->pp_link;
		}
		*tp[1] = 0;
		*tp[0] = pp2;
		page_free_list = pp1;
	}

	// if there's a page that shouldn't be on the free list,
	// try to make sure it eventually causes trouble.
	for (pp = page_free_list; pp; pp = pp->pp_link)
		if (PDX(page2pa(pp)) < pdx_limit)
			memset(page2kva(pp), 0x97, 128);

	first_free_page = (char *) boot_alloc(0);
	for (pp = page_free_list; pp; pp = pp->pp_link) {
		// check that we didn't corrupt the free list itself
		assert(pp >= pages);
		assert(pp < pages + npages);
		assert(((char *) pp - (char *) pages) % sizeof(*pp) == 0);

		// check a few pages that shouldn't be on the free list
		assert(page2pa(pp) != 0);
		assert(page2pa(pp) != IOPHYSMEM);
		assert(page2pa(pp) != EXTPHYSMEM - PGSIZE);
		assert(page2pa(pp) != EXTPHYSMEM);
		assert(page2pa(pp) < EXTPHYSMEM || (char *) page2kva(pp) >= first_free_page);

		if (page2pa(pp) < EXTPHYSMEM)
			++nfree_basemem;
		else
			++nfree_extmem;
	}

	assert(nfree_basemem > 0);
	assert(nfree_extmem > 0);

	cprintf("check_page_free_list() succeeded!\n");
}

//
// Check the physical page allocator (page_alloc(), page_free(),
// and page_init()).
//
static void
check_page_alloc(void)
{
	struct PageInfo *pp, *pp0, *pp1, *pp2;
	int nfree;
	struct PageInfo *fl;
	char *c;
	int i;

	if (!pages)
		panic("'pages' is a null pointer!");

	// check number of free pages
	for (pp = page_free_list, nfree = 0; pp; pp = pp->pp_link)
		++nfree;

	// should be able to allocate three pages
	pp0 = pp1 = pp2 = 0;
	assert((pp0 = page_alloc(0)));
	assert((pp1 = page_alloc(0)));
	assert((pp2 = page_alloc(0)));

	assert(pp0);
	assert(pp1 && pp1 != pp0);
	assert(pp2 && pp2 != pp1 && pp2 != pp0);
	assert(page2pa(pp0) < npages*PGSIZE);
	assert(page2pa(pp1) < npages*PGSIZE);
	assert(page2pa(pp2) < npages*PGSIZE);

	// temporarily steal the rest of the free pages
	fl = page_free_list;
	page_free_list = 0;

	// should be no free memory
	assert(!page_alloc(0));

	// free and re-allocate?
	page_free(pp0);
	page_free(pp1);
	page_free(pp2);
	pp0 = pp1 = pp2 = 0;
	assert((pp0 = page_alloc(0)));
	assert((pp1 = page_alloc(0)));
	assert((pp2 = page_alloc(0)));
	assert(pp0);
	assert(pp1 && pp1 != pp0);
	assert(pp2 && pp2 != pp1 && pp2 != pp0);
	assert(!page_alloc(0));

	// test flags
	memset(page2kva(pp0), 1, PGSIZE);
	page_free(pp0);
	assert((pp = page_alloc(ALLOC_ZERO)));
	assert(pp && pp0 == pp);
	c = page2kva(pp);
	for (i = 0; i < PGSIZE; i++)
		assert(c[i] == 0);

	// give free list back
	page_free_list = fl;

	// free the pages we took
	page_free(pp0);
	page_free(pp1);
	page_free(pp2);

	// number of free pages should be the same
	for (pp = page_free_list; pp; pp = pp->pp_link)
		--nfree;
	assert(nfree == 0);

	cprintf("check_page_alloc() succeeded!\n");
}

//
// Checks that the kernel part of virtual address space
// has been set up roughly correctly (by mem_init()).
//
// This function doesn't test every corner case,
// but it is a pretty good sanity check.
//

static void
check_kern_pgdir(void)
{
	uint32_t i, n;
	pde_t *pgdir;

	pgdir = kern_pgdir;

	// check pages array
	n = ROUNDUP(npages*sizeof(struct PageInfo), PGSIZE);
	for (i = 0; i < n; i += PGSIZE)
		assert(check_va2pa(pgdir, UPAGES + i) == PADDR(pages) + i);

	// check envs array (new test for lab 3)
	n = ROUNDUP(NENV*sizeof(struct Env), PGSIZE);
	for (i = 0; i < n; i += PGSIZE)
		assert(check_va2pa(pgdir, UENVS + i) == PADDR(envs) + i);

	// check phys mem
	for (i = 0; i < npages * PGSIZE; i += PGSIZE)
		assert(check_va2pa(pgdir, KERNBASE + i) == i);

	// check kernel stack
	for (i = 0; i < KSTKSIZE; i += PGSIZE)
		assert(check_va2pa(pgdir, KSTACKTOP - KSTKSIZE + i) == PADDR(bootstack) + i);
	assert(check_va2pa(pgdir, KSTACKTOP - PTSIZE) == ~0);

	// check PDE permissions
	for (i = 0; i < NPDENTRIES; i++) {
		switch (i) {
		case PDX(UVPT):
		case PDX(KSTACKTOP-1):
		case PDX(UPAGES):
		case PDX(UENVS):
			assert(pgdir[i] & PTE_P);
			break;
		default:
			if (i >= PDX(KERNBASE)) {
				assert(pgdir[i] & PTE_P);
				assert(pgdir[i] & PTE_W);
			} else
				assert(pgdir[i] == 0);
			break;
		}
	}
	cprintf("check_kern_pgdir() succeeded!\n");
}

// This function returns the physical address of the page containing 'va',
// defined by the page directory 'pgdir'.  The hardware normally performs
// this functionality for us!  We define our own version to help check
// the check_kern_pgdir() function; it shouldn't be used elsewhere.

static physaddr_t
check_va2pa(pde_t *pgdir, uintptr_t va)
{
	pte_t *p;

	pgdir = &pgdir[PDX(va)];
	if (!(*pgdir & PTE_P))
		return ~0;
	p = (pte_t*) KADDR(PTE_ADDR(*pgdir));
	if (!(p[PTX(va)] & PTE_P))
		return ~0;
	return PTE_ADDR(p[PTX(va)]);
}


// check page_insert, page_remove, &c
static void
check_page(void)
{
	struct PageInfo *pp, *pp0, *pp1, *pp2;
	struct PageInfo *fl;
	pte_t *ptep, *ptep1;
	void *va;
	int i;
	extern pde_t entry_pgdir[];

	// should be able to allocate three pages
	pp0 = pp1 = pp2 = 0;
	assert((pp0 = page_alloc(0)));
	assert((pp1 = page_alloc(0)));
	assert((pp2 = page_alloc(0)));

	assert(pp0);
	assert(pp1 && pp1 != pp0);
	assert(pp2 && pp2 != pp1 && pp2 != pp0);

	// temporarily steal the rest of the free pages
	fl = page_free_list;
	page_free_list = 0;

	// should be no free memory
	assert(!page_alloc(0));

	// there is no page allocated at address 0
	assert(page_lookup(kern_pgdir, (void *) 0x0, &ptep) == NULL);

	// there is no free memory, so we can't allocate a page table
	assert(page_insert(kern_pgdir, pp1, 0x0, PTE_W) < 0);

	// free pp0 and try again: pp0 should be used for page table
	page_free(pp0);
	assert(page_insert(kern_pgdir, pp1, 0x0, PTE_W) == 0);
	assert(PTE_ADDR(kern_pgdir[0]) == page2pa(pp0));
	assert(check_va2pa(kern_pgdir, 0x0) == page2pa(pp1));
	assert(pp1->pp_ref == 1);
	assert(pp0->pp_ref == 1);

	// should be able to map pp2 at PGSIZE because pp0 is already allocated for page table
	assert(page_insert(kern_pgdir, pp2, (void*) PGSIZE, PTE_W) == 0);
	assert(check_va2pa(kern_pgdir, PGSIZE) == page2pa(pp2));
	assert(pp2->pp_ref == 1);

	// should be no free memory
	assert(!page_alloc(0));

	// should be able to map pp2 at PGSIZE because it's already there
	assert(page_insert(kern_pgdir, pp2, (void*) PGSIZE, PTE_W) == 0);
	assert(check_va2pa(kern_pgdir, PGSIZE) == page2pa(pp2));
	assert(pp2->pp_ref == 1);

	// pp2 should NOT be on the free list
	// could happen in ref counts are handled sloppily in page_insert
	assert(!page_alloc(0));

	// check that pgdir_walk returns a pointer to the pte
	ptep = (pte_t *) KADDR(PTE_ADDR(kern_pgdir[PDX(PGSIZE)]));
	assert(pgdir_walk(kern_pgdir, (void*)PGSIZE, 0) == ptep+PTX(PGSIZE));

	// should be able to change permissions too.
	assert(page_insert(kern_pgdir, pp2, (void*) PGSIZE, PTE_W|PTE_U) == 0);
	assert(check_va2pa(kern_pgdir, PGSIZE) == page2pa(pp2));
	assert(pp2->pp_ref == 1);
	assert(*pgdir_walk(kern_pgdir, (void*) PGSIZE, 0) & PTE_U);
	assert(kern_pgdir[0] & PTE_U);

	// should be able to remap with fewer permissions
	assert(page_insert(kern_pgdir, pp2, (void*) PGSIZE, PTE_W) == 0);
	assert(*pgdir_walk(kern_pgdir, (void*) PGSIZE, 0) & PTE_W);
	assert(!(*pgdir_walk(kern_pgdir, (void*) PGSIZE, 0) & PTE_U));

	// should not be able to map at PTSIZE because need free page for page table
	assert(page_insert(kern_pgdir, pp0, (void*) PTSIZE, PTE_W) < 0);

	// insert pp1 at PGSIZE (replacing pp2)
	assert(page_insert(kern_pgdir, pp1, (void*) PGSIZE, PTE_W) == 0);
	assert(!(*pgdir_walk(kern_pgdir, (void*) PGSIZE, 0) & PTE_U));

	// should have pp1 at both 0 and PGSIZE, pp2 nowhere, ...
	assert(check_va2pa(kern_pgdir, 0) == page2pa(pp1));
	assert(check_va2pa(kern_pgdir, PGSIZE) == page2pa(pp1));
	// ... and ref counts should reflect this
	assert(pp1->pp_ref == 2);
	assert(pp2->pp_ref == 0);

	// pp2 should be returned by page_alloc
	assert((pp = page_alloc(0)) && pp == pp2);

	// unmapping pp1 at 0 should keep pp1 at PGSIZE
	page_remove(kern_pgdir, 0x0);
	assert(check_va2pa(kern_pgdir, 0x0) == ~0);
	assert(check_va2pa(kern_pgdir, PGSIZE) == page2pa(pp1));
	assert(pp1->pp_ref == 1);
	assert(pp2->pp_ref == 0);

	// test re-inserting pp1 at PGSIZE
	assert(page_insert(kern_pgdir, pp1, (void*) PGSIZE, 0) == 0);
	assert(pp1->pp_ref);
	assert(pp1->pp_link == NULL);

	// unmapping pp1 at PGSIZE should free it
	page_remove(kern_pgdir, (void*) PGSIZE);
	assert(check_va2pa(kern_pgdir, 0x0) == ~0);
	assert(check_va2pa(kern_pgdir, PGSIZE) == ~0);
	assert(pp1->pp_ref == 0);
	assert(pp2->pp_ref == 0);

	// so it should be returned by page_alloc
	assert((pp = page_alloc(0)) && pp == pp1);

	// should be no free memory
	assert(!page_alloc(0));

	// forcibly take pp0 back
	assert(PTE_ADDR(kern_pgdir[0]) == page2pa(pp0));
	kern_pgdir[0] = 0;
	assert(pp0->pp_ref == 1);
	pp0->pp_ref = 0;

	// check pointer arithmetic in pgdir_walk
	page_free(pp0);
	va = (void*)(PGSIZE * NPDENTRIES + PGSIZE);
	ptep = pgdir_walk(kern_pgdir, va, 1);
	ptep1 = (pte_t *) KADDR(PTE_ADDR(kern_pgdir[PDX(va)]));
	assert(ptep == ptep1 + PTX(va));
	kern_pgdir[PDX(va)] = 0;
	pp0->pp_ref = 0;

	// check that new page tables get cleared
	memset(page2kva(pp0), 0xFF, PGSIZE);
	page_free(pp0);
	pgdir_walk(kern_pgdir, 0x0, 1);
	ptep = (pte_t *) page2kva(pp0);
	for(i=0; i<NPTENTRIES; i++)
		assert((ptep[i] & PTE_P) == 0);
	kern_pgdir[0] = 0;
	pp0->pp_ref = 0;

	// give free list back
	page_free_list = fl;

	// free the pages we took
	page_free(pp0);
	page_free(pp1);
	page_free(pp2);

	cprintf("check_page() succeeded!\n");
}

// check page_insert, page_remove, &c, with an installed kern_pgdir
static void
check_page_installed_pgdir(void)
{
	struct PageInfo *pp, *pp0, *pp1, *pp2;
	struct PageInfo *fl;
	pte_t *ptep, *ptep1;
	uintptr_t va;
	int i;

	// check that we can read and write installed pages
	pp1 = pp2 = 0;
	assert((pp0 = page_alloc(0)));
	assert((pp1 = page_alloc(0)));
	assert((pp2 = page_alloc(0)));
	page_free(pp0);
	memset(page2kva(pp1), 1, PGSIZE);
	memset(page2kva(pp2), 2, PGSIZE);
	page_insert(kern_pgdir, pp1, (void*) PGSIZE, PTE_W);
	assert(pp1->pp_ref == 1);
	assert(*(uint32_t *)PGSIZE == 0x01010101U);
	page_insert(kern_pgdir, pp2, (void*) PGSIZE, PTE_W);
	assert(*(uint32_t *)PGSIZE == 0x02020202U);
	assert(pp2->pp_ref == 1);
	assert(pp1->pp_ref == 0);
	*(uint32_t *)PGSIZE = 0x03030303U;
	assert(*(uint32_t *)page2kva(pp2) == 0x03030303U);
	page_remove(kern_pgdir, (void*) PGSIZE);
	assert(pp2->pp_ref == 0);

	// forcibly take pp0 back
	assert(PTE_ADDR(kern_pgdir[0]) == page2pa(pp0));
	kern_pgdir[0] = 0;
	assert(pp0->pp_ref == 1);
	pp0->pp_ref = 0;

	// free the pages we took
	page_free(pp0);

	cprintf("check_page_installed_pgdir() succeeded!\n");
}
