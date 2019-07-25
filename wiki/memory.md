# 内存分布

![images/物理内存分布.png](../images/物理内存分布.png)

## 进程申请内存,操作系统处理的流程

1. 找到空闲的物理 page
2. 把指向这些物理 page 的 PTE 添加到进程的页表中
3. 设置这些 PTE 的 PTE_U, PTE_W, PTE_P 位
4. 未被使用的 PTE 的 PTE_P 位仍是 clear状态

## 内核的物理内存 allocator 是怎样工作的?

1. allocator 是一个 Page 链表,维护着所有 free 状态的物理内存 page.
2. 每个 free