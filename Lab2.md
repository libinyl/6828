# [Lab 2: 内存管理](https://pdos.csail.mit.edu/6.828/2018/labs/lab2/)

## 简介

内存管理包括两部分：

- 物理内存分配器，供内核使用。
    
  内核可以借助它来申请内存/释放内存。内存分配器将以 4096 字节为单位进行，这个单位被称为"页". 本实验维护一个数据结构，此结构将记录哪些物理页是释放状态，哪些是已申请状态，以及有多少个进程正在同时使用已申请的页。本实验还要完成申请/释放页内存的函数。

- 第二部分是虚拟内存 (virtual memory). 虚拟内存把内核和用户程序使用的虚拟地址映射到物理地址。当指令使用内存时，x86 硬件的内存管理单元 (MMU) 轮询页表 (page table), 来完成映射。本实验修改 JOS, 根据规范来建立 MMU 的页表。

## Part 1: 物理页管理

操作系统必须持续跟踪哪些物理 RAM 是占用或没占用的.JOS 通过**页面粒度**(page granulayity) 来管理物理内存.这样,操作系统就可以通过MMU来映射和保护每块内存.

物理页分配器(page allocator) 通过一个 `struct PageInfo` 的链表来跟踪哪些页是空闲的.每个`PageInfo`对象都与一个物理页相关.在写虚拟内存之前需要写好物理页分配器,因为页表管理需要物理页分配器来保存页表.

虚拟内存是"word access"的,即最小的访问单位是 word.

**练习 1** 






## 参考资料

[YouTube: Virtual Memory, David Black-Schaffer](https://www.youtube.com/watch?v=qcBIvnQt0Bw&list=PLiwt1iVUib9s2Uo5BeYmwkDFUh70fJPxX&index=1)