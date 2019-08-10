# [Lab 3: User Environments](https://pdos.csail.mit.edu/6.828/2018/labs/lab3/)

## Part A: User Environments and Exception Handling


**call graph:**

- start (kern/entry.S)
- i386_init (kern/init.c)
  - cons_init
  - mem_init
  - env_init
  - trap_init (still incomplete at this point)
  - env_create
  - env_run
    - env_pop_tf

类似 Unix 中的`process`概念,JOS 把`thread` 和`address space`的概念组合在一次.`thread` 主要由当前的寄存器值定义(由`env_tf`字段维护), `address space`由`page directory`和`page table`(被`env_pgdir`维护)定义. 要想运行某个环境,内核必须为 CPU 设置当前环境的寄存器值和合适的地址空间.

JOS中的`struct Env`与 xv6 中的`struct proc`类似. 它们都维护着当前环境/进程的用户模式的寄存器状态(在`Trapframe`中).在 JOS 中,每个环境没有维护各自的内核栈.而 xv6 中的进程是维护的. 在 JOS 中,任意时刻只有一个 env,所以 jos 只需要一个内核栈.

**练习 1** 修改`mem_init`, 申请并映射`env`数组. 此数组包含整整`NENV`个`Env`结构的实例.申请方式类似`pages`.保存`envs`的内存也应该映射至`UENVS`. 所以用户进程可以从这里读取.

## Creating and Running Environments

现在开始写一些为了运行用户环境而必需的的代码. 由于尚未建立文件系统,我们首先令内核加载一个与 kernel 本身绑定的静态二进制镜像.

Lab 3 的`GNUmakefile`生成了若干二进制文件,并放置到了`/obj/user/`目录中.


## 名次解释

**CPL** : **Current Privilege Level**, 当前进程的权限级别

**DPL** : **Descriptor Privilege Level**,

**PCB** : **Process Control Block** 进程控制表

## 参考资料

[CSDN: DPL,RPL,CPL 之间的联系和区别](https://blog.csdn.net/better0332/article/details/3416749)